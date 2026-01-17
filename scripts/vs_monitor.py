#!/usr/bin/env python3
"""
vs_monitor.py - IWR6843AOPEVM Vital Signs Monitor

Comprehensive CLI and data monitor for the vital signs firmware.
Handles both CLI command interface and TLV data parsing.

Usage:
    python vs_monitor.py                           # Auto-detect ports
    python vs_monitor.py --cli /dev/ttyUSB0 --data /dev/ttyUSB1
    python vs_monitor.py --config vital_signs_2m.cfg
    python vs_monitor.py --interactive

Author: IWR6843 RE Project
"""

import argparse
import struct
import serial
import serial.tools.list_ports
import sys
import time
import threading
import queue
import os
from datetime import datetime
from collections import deque

# =============================================================================
# Constants
# =============================================================================

# Frame sync magic word
MAGIC_WORD = bytes([0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07])

# TLV Types
TLV_TYPES = {
    1: "DETECTED_POINTS",
    2: "RANGE_PROFILE",
    3: "NOISE_PROFILE",
    4: "AZIMUTH_STATIC_HEATMAP",
    5: "RANGE_DOPPLER_HEATMAP",
    6: "STATS",
    7: "DETECTED_POINTS_SIDE_INFO",
    8: "AZIMUTH_ELEVATION_STATIC_HEATMAP",
    9: "TEMPERATURE_STATS",
    0x410: "VITAL_SIGNS",
}

# Header size (magic + header fields)
HEADER_SIZE = 40

# ANSI colors for terminal output
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[2m'

# =============================================================================
# Data Structures
# =============================================================================

class FrameHeader:
    """mmWave frame header parser"""
    def __init__(self, data):
        if len(data) < HEADER_SIZE:
            raise ValueError(f"Header too short: {len(data)} bytes")

        offset = 8  # Skip magic word
        self.version = struct.unpack('<I', data[offset:offset+4])[0]
        self.total_packet_len = struct.unpack('<I', data[offset+4:offset+8])[0]
        self.platform = struct.unpack('<I', data[offset+8:offset+12])[0]
        self.frame_number = struct.unpack('<I', data[offset+12:offset+16])[0]
        self.time_cpu_cycles = struct.unpack('<I', data[offset+16:offset+20])[0]
        self.num_detected_obj = struct.unpack('<I', data[offset+20:offset+24])[0]
        self.num_tlvs = struct.unpack('<I', data[offset+24:offset+28])[0]
        self.subframe_number = struct.unpack('<I', data[offset+28:offset+32])[0]


class VitalSignsData:
    """Vital signs TLV data parser"""
    def __init__(self, data):
        if len(data) < 20:
            raise ValueError(f"VS data too short: {len(data)} bytes")

        self.target_id = struct.unpack('<H', data[0:2])[0]
        self.range_bin = struct.unpack('<H', data[2:4])[0]
        self.heart_rate = struct.unpack('<f', data[4:8])[0]
        self.breathing_rate = struct.unpack('<f', data[8:12])[0]
        self.breathing_deviation = struct.unpack('<f', data[12:16])[0]
        self.valid = data[16]
        # reserved[3] at bytes 17-19

    def __str__(self):
        status = f"{Colors.GREEN}VALID{Colors.ENDC}" if self.valid else f"{Colors.RED}INVALID{Colors.ENDC}"
        return (f"[{status}] HR={self.heart_rate:6.1f} BPM | "
                f"BR={self.breathing_rate:5.1f} BPM | "
                f"Dev={self.breathing_deviation:.4f} | "
                f"Bin={self.range_bin:3d} | ID={self.target_id}")


class DetectedPoint:
    """Detected object point parser"""
    def __init__(self, data, offset=0):
        self.x = struct.unpack('<f', data[offset:offset+4])[0]
        self.y = struct.unpack('<f', data[offset+4:offset+8])[0]
        self.z = struct.unpack('<f', data[offset+8:offset+12])[0]
        self.doppler = struct.unpack('<f', data[offset+12:offset+16])[0]

    @property
    def range(self):
        return (self.x**2 + self.y**2 + self.z**2)**0.5

    def __str__(self):
        return f"({self.x:6.2f}, {self.y:6.2f}, {self.z:6.2f}) R={self.range:5.2f}m V={self.doppler:5.2f}m/s"


# =============================================================================
# Serial Port Management
# =============================================================================

def find_mmwave_ports():
    """Auto-detect mmWave serial ports"""
    ports = list(serial.tools.list_ports.comports())
    mmwave_ports = []

    for port in ports:
        # Look for Silicon Labs CP210x (common on mmWave EVMs)
        if 'CP210' in str(port.description) or 'Silicon Labs' in str(port.manufacturer or ''):
            mmwave_ports.append(port.device)
        # Also check for generic USB serial
        elif 'ttyUSB' in port.device or 'ttyACM' in port.device:
            mmwave_ports.append(port.device)

    mmwave_ports.sort()
    return mmwave_ports


def open_serial_port(port, baud, timeout=0.1):
    """Open serial port with error handling"""
    try:
        ser = serial.Serial(port, baud, timeout=timeout)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        return ser
    except serial.SerialException as e:
        print(f"{Colors.RED}Error opening {port}: {e}{Colors.ENDC}")
        return None


# =============================================================================
# CLI Interface
# =============================================================================

class CLIInterface:
    """Handle CLI command interface"""

    def __init__(self, port, baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None
        self.response_queue = queue.Queue()
        self.running = False
        self.reader_thread = None

    def connect(self):
        """Connect to CLI port"""
        self.ser = open_serial_port(self.port, self.baud)
        if self.ser:
            self.running = True
            self.reader_thread = threading.Thread(target=self._reader_loop, daemon=True)
            self.reader_thread.start()
            return True
        return False

    def disconnect(self):
        """Disconnect from CLI port"""
        self.running = False
        if self.reader_thread:
            self.reader_thread.join(timeout=1)
        if self.ser:
            self.ser.close()
            self.ser = None

    def _reader_loop(self):
        """Background thread to read CLI responses"""
        buffer = ""
        while self.running and self.ser:
            try:
                data = self.ser.read(1024)
                if data:
                    buffer += data.decode('utf-8', errors='replace')
                    # Process complete lines
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        if line:
                            self.response_queue.put(line)
            except Exception as e:
                if self.running:
                    print(f"{Colors.RED}CLI read error: {e}{Colors.ENDC}")
                break

    def send_command(self, cmd, wait_response=True, timeout=1.0):
        """Send CLI command and optionally wait for response"""
        if not self.ser:
            return None

        # Clear any pending responses
        while not self.response_queue.empty():
            try:
                self.response_queue.get_nowait()
            except queue.Empty:
                break

        # Send command
        self.ser.write((cmd + '\n').encode())
        self.ser.flush()

        if not wait_response:
            return None

        # Collect response
        responses = []
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                response = self.response_queue.get(timeout=0.1)
                responses.append(response)
            except queue.Empty:
                if responses:  # Got something, wait a bit more then return
                    time.sleep(0.1)
                    while not self.response_queue.empty():
                        try:
                            responses.append(self.response_queue.get_nowait())
                        except queue.Empty:
                            break
                    break

        return responses

    def send_config_file(self, filepath):
        """Send configuration file to device"""
        if not os.path.exists(filepath):
            print(f"{Colors.RED}Config file not found: {filepath}{Colors.ENDC}")
            return False

        print(f"{Colors.CYAN}Sending configuration: {filepath}{Colors.ENDC}")

        with open(filepath, 'r') as f:
            lines = f.readlines()

        for line in lines:
            line = line.strip()
            # Skip comments and empty lines
            if not line or line.startswith('%') or line.startswith('#'):
                continue

            print(f"  > {line}")
            responses = self.send_command(line, timeout=0.5)

            if responses:
                for r in responses:
                    if 'error' in r.lower() or 'invalid' in r.lower():
                        print(f"    {Colors.RED}{r}{Colors.ENDC}")
                    elif 'done' in r.lower():
                        print(f"    {Colors.GREEN}{r}{Colors.ENDC}")
                    else:
                        print(f"    {Colors.DIM}{r}{Colors.ENDC}")

            time.sleep(0.05)  # Small delay between commands

        print(f"{Colors.GREEN}Configuration sent{Colors.ENDC}")
        return True


# =============================================================================
# Data Stream Parser
# =============================================================================

class DataStreamParser:
    """Parse mmWave data frames from serial stream"""

    def __init__(self, port, baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        self.frame_queue = queue.Queue(maxsize=100)
        self.parser_thread = None

        # Statistics
        self.frames_received = 0
        self.frames_with_vs = 0
        self.last_frame_time = None
        self.frame_times = deque(maxlen=100)

    def connect(self):
        """Connect to data port"""
        self.ser = open_serial_port(self.port, self.baud)
        if self.ser:
            self.running = True
            self.parser_thread = threading.Thread(target=self._parser_loop, daemon=True)
            self.parser_thread.start()
            return True
        return False

    def disconnect(self):
        """Disconnect from data port"""
        self.running = False
        if self.parser_thread:
            self.parser_thread.join(timeout=1)
        if self.ser:
            self.ser.close()
            self.ser = None

    def _find_magic_word(self, timeout=1.0):
        """Find magic word in stream"""
        buffer = b''
        start_time = time.time()

        while self.running and time.time() - start_time < timeout:
            if self.ser.in_waiting:
                buffer += self.ser.read(self.ser.in_waiting)
                idx = buffer.find(MAGIC_WORD)
                if idx >= 0:
                    return buffer[idx:]
            time.sleep(0.001)

        return None

    def _read_frame(self):
        """Read a complete frame"""
        # Find magic word
        data = self._find_magic_word()
        if data is None:
            return None

        # Read header if needed
        while len(data) < HEADER_SIZE and self.running:
            if self.ser.in_waiting:
                data += self.ser.read(min(HEADER_SIZE - len(data), self.ser.in_waiting))
            time.sleep(0.001)

        if len(data) < HEADER_SIZE:
            return None

        # Parse header to get length
        try:
            header = FrameHeader(data)
        except ValueError:
            return None

        # Read rest of frame
        while len(data) < header.total_packet_len and self.running:
            if self.ser.in_waiting:
                remaining = header.total_packet_len - len(data)
                data += self.ser.read(min(remaining, self.ser.in_waiting))
            time.sleep(0.001)

        return data

    def _parse_tlvs(self, data, header):
        """Parse TLVs from frame"""
        tlvs = {}
        offset = HEADER_SIZE

        for _ in range(header.num_tlvs):
            if offset + 8 > len(data):
                break

            tlv_type = struct.unpack('<I', data[offset:offset+4])[0]
            tlv_length = struct.unpack('<I', data[offset+4:offset+8])[0]
            offset += 8

            if offset + tlv_length > len(data):
                break

            tlvs[tlv_type] = data[offset:offset+tlv_length]
            offset += tlv_length

        return tlvs

    def _parser_loop(self):
        """Background thread to parse frames"""
        while self.running:
            try:
                data = self._read_frame()
                if data is None:
                    continue

                header = FrameHeader(data)
                tlvs = self._parse_tlvs(data, header)

                # Update statistics
                now = time.time()
                if self.last_frame_time:
                    self.frame_times.append(now - self.last_frame_time)
                self.last_frame_time = now
                self.frames_received += 1

                if 0x410 in tlvs:
                    self.frames_with_vs += 1

                # Queue frame for processing
                try:
                    self.frame_queue.put_nowait((header, tlvs))
                except queue.Full:
                    self.frame_queue.get()  # Drop oldest
                    self.frame_queue.put_nowait((header, tlvs))

            except Exception as e:
                if self.running:
                    print(f"{Colors.RED}Parse error: {e}{Colors.ENDC}")

    def get_frame(self, timeout=0.1):
        """Get next parsed frame"""
        try:
            return self.frame_queue.get(timeout=timeout)
        except queue.Empty:
            return None

    @property
    def frame_rate(self):
        """Calculate current frame rate"""
        if len(self.frame_times) < 2:
            return 0
        avg_interval = sum(self.frame_times) / len(self.frame_times)
        return 1.0 / avg_interval if avg_interval > 0 else 0


# =============================================================================
# Monitor Display
# =============================================================================

class VitalSignsMonitor:
    """Main monitor application"""

    def __init__(self, cli_port, data_port, cli_baud=115200, data_baud=921600):
        self.cli = CLIInterface(cli_port, cli_baud) if cli_port else None
        self.data = DataStreamParser(data_port, data_baud) if data_port else None

        self.running = False
        self.display_mode = 'vital_signs'  # vital_signs, all_tlvs, stats

        # Vital signs history for averaging
        self.hr_history = deque(maxlen=10)
        self.br_history = deque(maxlen=10)

        # Logging
        self.log_file = None

    def connect(self):
        """Connect to both ports"""
        success = True

        if self.cli:
            if self.cli.connect():
                print(f"{Colors.GREEN}CLI connected: {self.cli.port}{Colors.ENDC}")
            else:
                print(f"{Colors.RED}CLI connection failed{Colors.ENDC}")
                success = False

        if self.data:
            if self.data.connect():
                print(f"{Colors.GREEN}Data connected: {self.data.port}{Colors.ENDC}")
            else:
                print(f"{Colors.RED}Data connection failed{Colors.ENDC}")
                success = False

        return success

    def disconnect(self):
        """Disconnect from both ports"""
        if self.cli:
            self.cli.disconnect()
        if self.data:
            self.data.disconnect()
        if self.log_file:
            self.log_file.close()

    def start_logging(self, filepath):
        """Start logging to CSV file"""
        self.log_file = open(filepath, 'w')
        self.log_file.write("timestamp,frame,heart_rate,breathing_rate,deviation,valid,range_bin,target_id\n")
        print(f"{Colors.CYAN}Logging to: {filepath}{Colors.ENDC}")

    def _log_vital_signs(self, frame_num, vs):
        """Log vital signs data to file"""
        if self.log_file:
            timestamp = datetime.now().isoformat()
            self.log_file.write(f"{timestamp},{frame_num},{vs.heart_rate:.2f},"
                               f"{vs.breathing_rate:.2f},{vs.breathing_deviation:.6f},"
                               f"{vs.valid},{vs.range_bin},{vs.target_id}\n")
            self.log_file.flush()

    def _print_header(self):
        """Print display header"""
        print(f"\n{Colors.BOLD}{'='*70}{Colors.ENDC}")
        print(f"{Colors.BOLD}  IWR6843AOPEVM Vital Signs Monitor{Colors.ENDC}")
        print(f"{Colors.BOLD}{'='*70}{Colors.ENDC}")
        if self.data:
            print(f"  Data Port: {self.data.port} @ {self.data.baud} baud")
        if self.cli:
            print(f"  CLI Port:  {self.cli.port} @ {self.cli.baud} baud")
        print(f"{'='*70}\n")

    def _print_vital_signs(self, frame_num, vs):
        """Print vital signs with formatting"""
        # Update history for averaging
        if vs.valid:
            self.hr_history.append(vs.heart_rate)
            self.br_history.append(vs.breathing_rate)

        # Calculate averages
        avg_hr = sum(self.hr_history) / len(self.hr_history) if self.hr_history else 0
        avg_br = sum(self.br_history) / len(self.br_history) if self.br_history else 0

        # Clear line and print
        print(f"\r{Colors.CYAN}Frame {frame_num:6d}{Colors.ENDC} | {vs} | "
              f"Avg: HR={avg_hr:5.1f} BR={avg_br:4.1f}", end='', flush=True)

    def _print_stats(self):
        """Print statistics"""
        if self.data:
            print(f"\n{Colors.YELLOW}--- Statistics ---{Colors.ENDC}")
            print(f"  Frames received: {self.data.frames_received}")
            print(f"  Frames with VS:  {self.data.frames_with_vs}")
            print(f"  Frame rate:      {self.data.frame_rate:.1f} Hz")
            if self.hr_history:
                print(f"  Avg Heart Rate:  {sum(self.hr_history)/len(self.hr_history):.1f} BPM")
            if self.br_history:
                print(f"  Avg Breath Rate: {sum(self.br_history)/len(self.br_history):.1f} BPM")

    def run_monitor(self):
        """Run the main monitor loop"""
        self._print_header()
        self.running = True

        print(f"{Colors.CYAN}Monitoring... Press Ctrl+C to stop{Colors.ENDC}\n")

        try:
            while self.running:
                if not self.data:
                    time.sleep(0.1)
                    continue

                frame_data = self.data.get_frame(timeout=0.1)
                if frame_data is None:
                    continue

                header, tlvs = frame_data

                # Process vital signs TLV
                if 0x410 in tlvs:
                    try:
                        vs = VitalSignsData(tlvs[0x410])
                        self._print_vital_signs(header.frame_number, vs)
                        self._log_vital_signs(header.frame_number, vs)
                    except ValueError as e:
                        print(f"\r{Colors.RED}VS parse error: {e}{Colors.ENDC}")

                # Show other TLVs in verbose mode
                elif self.display_mode == 'all_tlvs':
                    tlv_names = [TLV_TYPES.get(t, f"0x{t:X}") for t in tlvs.keys()]
                    print(f"\rFrame {header.frame_number:6d} | Objects: {header.num_detected_obj:3d} | "
                          f"TLVs: {', '.join(tlv_names)}", end='', flush=True)

        except KeyboardInterrupt:
            print(f"\n\n{Colors.YELLOW}Stopping...{Colors.ENDC}")

        finally:
            self.running = False
            self._print_stats()

    def run_interactive(self):
        """Run interactive CLI mode"""
        if not self.cli:
            print(f"{Colors.RED}No CLI port configured{Colors.ENDC}")
            return

        self._print_header()

        print(f"{Colors.CYAN}Interactive CLI mode. Commands:{Colors.ENDC}")
        print("  load [file]   - Load config (default: vital_signs_2m.cfg)")
        print("  start         - Start sensor")
        print("  stop          - Stop sensor")
        print("  vs_enable     - Enable vital signs (vitalsign 1 0)")
        print("  vs_disable    - Disable vital signs (vitalsign 0 0)")
        print("  vs_range N M  - Set range bin config (VSRangeIdxCfg N M)")
        print("  status        - Show parser statistics")
        print("  help          - Show all CLI commands")
        print("  quit          - Exit")
        print()

        # Start data parser in background if available
        if self.data:
            threading.Thread(target=self._background_data_display, daemon=True).start()

        try:
            while True:
                try:
                    cmd = input(f"{Colors.GREEN}> {Colors.ENDC}").strip()
                except EOFError:
                    break

                if not cmd:
                    continue

                if cmd == 'quit' or cmd == 'exit':
                    break
                elif cmd.startswith('load'):
                    parts = cmd.split(maxsplit=1)
                    if len(parts) > 1:
                        cfg_file = parts[1]
                    else:
                        # Default config file
                        script_dir = os.path.dirname(os.path.abspath(__file__))
                        cfg_file = os.path.join(script_dir, '..', 'configs', 'chirp_profiles', 'vital_signs_2m.cfg')
                    if os.path.exists(cfg_file):
                        self.cli.send_config_file(cfg_file)
                    else:
                        print(f"{Colors.RED}Config not found: {cfg_file}{Colors.ENDC}")
                elif cmd == 'start':
                    responses = self.cli.send_command('sensorStart')
                    if responses:
                        for r in responses:
                            if 'error' in r.lower():
                                print(f"  {Colors.RED}{r}{Colors.ENDC}")
                            else:
                                print(f"  {Colors.GREEN}{r}{Colors.ENDC}")
                elif cmd == 'stop':
                    self.cli.send_command('sensorStop')
                    print("Sensor stopped")
                elif cmd == 'help':
                    self._show_help()
                elif cmd == 'vs_enable':
                    self.cli.send_command('vitalsign 1 0')
                    print("Vital signs enabled")
                elif cmd == 'vs_disable':
                    self.cli.send_command('vitalsign 0 0')
                    print("Vital signs disabled")
                elif cmd.startswith('vs_range'):
                    parts = cmd.split()
                    if len(parts) == 3:
                        self.cli.send_command(f'VSRangeIdxCfg {parts[1]} {parts[2]}')
                    else:
                        print("Usage: vs_range <start_bin> <num_bins>")
                elif cmd == 'status':
                    self._print_stats()
                else:
                    # Send raw command
                    responses = self.cli.send_command(cmd)
                    if responses:
                        for r in responses:
                            print(f"  {r}")

        except KeyboardInterrupt:
            print()

    def _background_data_display(self):
        """Background thread to display data while in interactive mode"""
        while self.running:
            if not self.data:
                break
            frame_data = self.data.get_frame(timeout=0.5)
            if frame_data:
                header, tlvs = frame_data
                if 0x410 in tlvs:
                    try:
                        vs = VitalSignsData(tlvs[0x410])
                        # Print on new line to not interfere with input
                        print(f"\n  {Colors.DIM}[Frame {header.frame_number}] {vs}{Colors.ENDC}")
                    except:
                        pass

    def _show_help(self):
        """Show CLI help"""
        print(f"\n{Colors.CYAN}Available Commands:{Colors.ENDC}")
        print("  sensorStop              - Stop sensor")
        print("  sensorStart             - Start sensor")
        print("  flushCfg                - Flush configuration")
        print("  vitalsign <en> <track>  - Enable/configure vital signs")
        print("  VSRangeIdxCfg <s> <n>   - Set range bin (start, count)")
        print("  VSTargetId <id>         - Set target ID (255=nearest)")
        print("  version                 - Show firmware version")
        print()


# =============================================================================
# Main Entry Point
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='IWR6843AOPEVM Vital Signs Monitor',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                                    # Auto-detect ports, monitor mode
  %(prog)s --cli /dev/ttyUSB0 --data /dev/ttyUSB1
  %(prog)s --config configs/chirp_profiles/vital_signs_2m.cfg
  %(prog)s --interactive                      # Interactive CLI mode
  %(prog)s --log vitalsigns.csv               # Log data to CSV
        """
    )

    parser.add_argument('--cli', help='CLI serial port (e.g., /dev/ttyUSB0)')
    parser.add_argument('--data', help='Data serial port (e.g., /dev/ttyUSB1)')
    parser.add_argument('--cli-baud', type=int, default=115200, help='CLI baud rate')
    parser.add_argument('--data-baud', type=int, default=921600, help='Data baud rate')
    parser.add_argument('--config', help='Send config file before monitoring')
    parser.add_argument('--log', help='Log vital signs to CSV file')
    parser.add_argument('--interactive', '-i', action='store_true', help='Interactive CLI mode')
    parser.add_argument('--all-tlvs', action='store_true', help='Show all TLV types')

    args = parser.parse_args()

    # Auto-detect ports if not specified
    cli_port = args.cli
    data_port = args.data

    if not cli_port or not data_port:
        ports = find_mmwave_ports()
        if len(ports) >= 2:
            if not cli_port:
                cli_port = ports[0]
            if not data_port:
                data_port = ports[1]
            print(f"{Colors.CYAN}Auto-detected ports: CLI={cli_port}, Data={data_port}{Colors.ENDC}")
        elif len(ports) == 1:
            if not cli_port:
                cli_port = ports[0]
            print(f"{Colors.YELLOW}Only one port detected: {cli_port}{Colors.ENDC}")
        else:
            print(f"{Colors.RED}No mmWave serial ports detected{Colors.ENDC}")
            print("Please specify ports with --cli and --data")
            sys.exit(1)

    # Create monitor
    monitor = VitalSignsMonitor(cli_port, data_port, args.cli_baud, args.data_baud)

    # Connect
    if not monitor.connect():
        print(f"{Colors.RED}Failed to connect{Colors.ENDC}")
        sys.exit(1)

    try:
        # Send config file if specified
        if args.config and monitor.cli:
            monitor.cli.send_config_file(args.config)
            time.sleep(0.5)

        # Start logging if specified
        if args.log:
            monitor.start_logging(args.log)

        # Set display mode
        if args.all_tlvs:
            monitor.display_mode = 'all_tlvs'

        # Run appropriate mode
        if args.interactive:
            monitor.run_interactive()
        else:
            monitor.run_monitor()

    finally:
        monitor.disconnect()
        print(f"{Colors.GREEN}Disconnected{Colors.ENDC}")


if __name__ == '__main__':
    main()
