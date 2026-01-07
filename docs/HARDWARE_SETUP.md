# Hardware Setup

## Supported Boards

- **IWR6843AOPEVM** - Antenna-on-Package evaluation module (primary target)
- **IWR6843ISK** - Integrated Sensor Kit

## Connections

### USB Connection

Connect USB cable to the Micro-USB port. This provides:
- Power to the board
- Two virtual COM ports:
  - CLI port (typically `/dev/ttyACM0` or `COM3`)
  - Data port (typically `/dev/ttyACM1` or `COM4`)

### Identifying Ports

**Linux:**
```bash
ls /dev/ttyACM*
# or
dmesg | grep ttyACM
```

**Windows:**
Check Device Manager → Ports (COM & LPT)

## SOP Switch Settings

The SOP (Sense On Power) switches control boot mode.

### IWR6843AOPEVM

| Mode | S1.3 (SOP0) | S1.4 (SOP1) | S3 (SOP2) |
|------|-------------|-------------|-----------|
| **Run** | OFF | OFF | OFF |
| **Flash** | OFF | OFF | ON |

**Important:** Power cycle after changing switches.

### Flash Mode

1. Set switches to Flash mode
2. Power cycle
3. Run flash command
4. Set switches to Run mode
5. Power cycle

## LED Indicators

- **Green LED**: Power
- **Blue LED**: Sensor running
- **Red LED**: Error condition

## Mounting

For vital signs monitoring:
- Mount radar facing the subject
- Distance: 0.5 - 2.5 meters
- Orientation: Horizontal or vertical
- Avoid metallic objects in field of view
