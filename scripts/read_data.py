import serial
import struct
import time

# CONFIG: Change to your data port (usually USB1 if CLI is USB0)
DATA_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200

def parse_radar_data():
    try:
        # Open the data port
        ser = serial.Serial(DATA_PORT, BAUD_RATE, timeout=1)
        print(f"Listening on {DATA_PORT}...")
        
        while True:
            # 1. Read the stream looking for the Magic Word (Sync Pattern)
            # The sync pattern is always: 02 01 04 03 06 05 08 07
            data = ser.read_until(b'\x02\x01\x04\x03\x06\x05\x08\x07')
            
            # If we timed out or didn't find the pattern, loop
            if not data or len(data) < 8:
                continue
                
            # 2. We found the sync! The next 32 bytes are the Frame Header.
            # We need to read the rest of the header (Total header is 40 bytes, we read 8 already)
            header_data = ser.read(32) 
            if len(header_data) < 32: continue

            # 3. Unpack the header (Standard TI SDK Header Format)
            # struct format: I (Version), I (TotalLen), I (Platform), I (FrameNum), 
            #                I (Time), I (NumDetObj), I (NumTLVs), I (SubFrame)
            try:
                frame_header = struct.unpack('<8I', header_data)
                version, total_len, platform, frame_num, cpu_cycles, num_obj, num_tlvs, subframe = frame_header
                
                print(f"Frame: {frame_num} | Detected Objects: {num_obj} | Total Packet Size: {total_len}")
                
            except struct.error:
                print("Header parse error")

    except KeyboardInterrupt:
        print("\nStopping...")
        ser.close()
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    parse_radar_data()