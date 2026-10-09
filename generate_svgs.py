import os

def create_svg(filename, title, pins_left, pins_right, width=500, height=400):
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" style="background-color: white; font-family: monospace;">
    <rect x="150" y="50" width="200" height="{max(len(pins_left), len(pins_right)) * 40 + 60}" fill="#f0f0f0" stroke="black" stroke-width="2"/>
    <text x="250" y="80" font-size="16" font-weight="bold" text-anchor="middle">{title}</text>
'''
    y = 120
    for pin in pins_left:
        svg += f'''
    <line x1="50" y1="{y}" x2="150" y2="{y}" stroke="black" stroke-width="2"/>
    <circle cx="50" cy="{y}" r="3" fill="black"/>
    <text x="155" y="{y+4}" font-size="12" fill="blue">{pin[0]}</text>
    <text x="45" y="{y-5}" font-size="12" text-anchor="end">{pin[1]}</text>
'''
        y += 40

    y = 120
    for pin in pins_right:
        svg += f'''
    <line x1="350" y1="{y}" x2="450" y2="{y}" stroke="black" stroke-width="2"/>
    <circle cx="450" cy="{y}" r="3" fill="black"/>
    <text x="345" y="{y+4}" font-size="12" fill="blue" text-anchor="end">{pin[0]}</text>
    <text x="455" y="{y-5}" font-size="12">{pin[1]}</text>
'''
        y += 40
        
    svg += '''
    <text x="250" y="380" font-size="10" fill="gray" text-anchor="middle">Note: External components TBD - Hardware Verification Required</text>
    </svg>'''
    
    with open(filename, 'w') as f:
        f.write(svg)

os.makedirs('docs/circuit-diagrams', exist_ok=True)

# Overall Diagram
create_svg('docs/circuit-diagrams/overall-circuit.svg', 'EV Powertrain System',
           [('CAN_RX', 'CAN Bus Network'), ('CAN_TX', 'CAN Bus Network')],
           [('MCU_EN', 'MCU System'), ('BMS_CS', 'BMS System')], width=600, height=400)

# MCU Diagram
create_svg('docs/circuit-diagrams/mcu-circuit.svg', 'MCU (STM32F103)',
           [('PA0', 'Hall Sensor IN'), ('PA1', 'Potentiometer IN'), ('PA11', 'CAN_RX (TBD Transceiver)'), ('PB10', 'START Button IN'), ('PB11', 'STOP Button IN')],
           [('PA8', 'PWM OUT (Gate Driver TBD)'), ('PB0', 'DIR1 OUT'), ('PB1', 'DIR2 OUT'), ('PA12', 'CAN_TX (TBD)'), ('PB8', 'STATUS LED OUT')], width=600, height=450)

# VCU Diagram
create_svg('docs/circuit-diagrams/vcu-circuit.svg', 'VCU (STM32F103)',
           [('PA0', 'Throttle ADC (0-3.3V)'), ('PA1', 'Brake ADC (0-3.3V)'), ('PB10', 'Dashboard SW IN'), ('PA11', 'CAN_RX')],
           [('PB8', 'Motor EN Relay'), ('PB9', 'Contactor EN Relay'), ('PB6', 'I2C_SCL'), ('PA12', 'CAN_TX')], width=600, height=400)

# BMS Diagram
create_svg('docs/circuit-diagrams/bms-circuit.svg', 'BMS (STM32F103)',
           [('PA0', 'V_Sense (Divider TBD)'), ('PA1', 'I_Sense (Shunt TBD)'), ('PA2', 'T_Sense (NTC TBD)'), ('PA11', 'CAN_RX')],
           [('PB0', 'Status LED'), ('PB1', 'Fault LED'), ('PA12', 'CAN_TX')], width=600, height=400)

