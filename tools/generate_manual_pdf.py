#!/usr/bin/env python3
"""
Coriolis — Hardware Manual & Operating Guide PDF Generator
==========================================================
Generates a publication-grade PDF manual covering the complete Bill of Materials
(BOM), mechanical assembly, power distribution, wiring schematics, firmware
flashing, and user operating guide.
"""

import os
from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.pdfgen import canvas

# Palette constants
PRIMARY = colors.HexColor("#0B192C")      # Deep Navy
SECONDARY = colors.HexColor("#007869")    # Coriolis Teal
ACCENT = colors.HexColor("#00AF96")       # Bright Teal
TEXT_DARK = colors.HexColor("#1A1A1A")    # Charcoal body text
TEXT_MUTED = colors.HexColor("#555555")   # Muted gray
BG_LIGHT = colors.HexColor("#F4F7F9")     # Light card background
BORDER_COLOR = colors.HexColor("#D1D9E0") # Table border gray


class NumberedCanvas(canvas.Canvas):
    """Two-pass canvas to dynamically compute and print total page count."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_page_decorations(self, total_pages):
        # Skip header and footer on cover page
        if self._pageNumber == 1:
            return

        self.saveState()
        self.setFont("Helvetica", 8)
        self.setFillColor(TEXT_MUTED)

        # Running top header
        self.drawString(54, 800, "CORIOLIS — 128×128 LED Matrix Wall Display & Guided Fitness Appliance")
        self.setStrokeColor(BORDER_COLOR)
        self.setLineWidth(0.5)
        self.line(54, 792, 541, 792)

        # Running bottom footer
        self.line(54, 45, 541, 45)
        self.drawString(54, 32, "Hardware Manual & Operating Guide — Final Edition (October 2026)")
        page_str = f"Page {self._pageNumber} of {total_pages}"
        self.drawRightString(541, 32, page_str)
        self.restoreState()


def build_pdf(filename="docs/CORIOLIS_HARDWARE_MANUAL.pdf"):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    doc = SimpleDocTemplate(
        filename,
        pagesize=A4,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )

    styles = getSampleStyleSheet()

    title_style = ParagraphStyle(
        'CoverTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=32,
        leading=38,
        textColor=PRIMARY,
        spaceAfter=8
    )

    subtitle_style = ParagraphStyle(
        'CoverSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=14,
        leading=18,
        textColor=SECONDARY,
        spaceAfter=20
    )

    h1_style = ParagraphStyle(
        'Heading1_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=18,
        leading=22,
        textColor=PRIMARY,
        spaceBefore=14,
        spaceAfter=8,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'Heading2_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=13,
        leading=16,
        textColor=SECONDARY,
        spaceBefore=10,
        spaceAfter=5,
        keepWithNext=True
    )

    h3_style = ParagraphStyle(
        'Heading3_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=10,
        leading=13,
        textColor=PRIMARY,
        spaceBefore=6,
        spaceAfter=3,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=9,
        leading=13,
        textColor=TEXT_DARK,
        spaceAfter=6
    )

    body_bold = ParagraphStyle(
        'Body_Bold',
        parent=body_style,
        fontName='Helvetica-Bold'
    )

    callout_style = ParagraphStyle(
        'Callout',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.5,
        leading=12,
        textColor=PRIMARY
    )

    table_header_style = ParagraphStyle(
        'TableHeader',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=8.5,
        leading=11,
        textColor=colors.white
    )

    table_cell_style = ParagraphStyle(
        'TableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=10.5,
        textColor=TEXT_DARK
    )

    table_cell_bold = ParagraphStyle(
        'TableCellBold',
        parent=table_cell_style,
        fontName='Helvetica-Bold'
    )

    story = []

    # =========================================================================
    # COVER PAGE
    # =========================================================================
    story.append(Spacer(1, 40))
    story.append(HRFlowable(width="100%", thickness=6, color=ACCENT, spaceAfter=20))
    story.append(Paragraph("CORIOLIS", title_style))
    story.append(Paragraph("128×128 RGB LED Matrix Wall Display & Guided Fitness Appliance", subtitle_style))
    story.append(HRFlowable(width="100%", thickness=1, color=BORDER_COLOR, spaceAfter=20))

    cover_meta = [
        [Paragraph("<b>Author / Project:</b>", body_style), Paragraph("Santtu Nykänen / Coriolis Project", body_style)],
        [Paragraph("<b>Document Version:</b>", body_style), Paragraph("1.0 (Hardware Release & Builder's Edition)", body_style)],
        [Paragraph("<b>Date:</b>", body_style), Paragraph("October 2026", body_style)],
        [Paragraph("<b>Architecture:</b>", body_style), Paragraph("Teensy 4.1 + SmartLED Shield V5 (HUB75 128×128)", body_style)],
        [Paragraph("<b>Codebase:</b>", body_style), Paragraph("https://github.com/cyanidesayonara/coriolis", body_style)],
    ]
    meta_table = Table(cover_meta, colWidths=[130, 350])
    meta_table.setStyle(TableStyle([
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('TOPPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(meta_table)

    story.append(Spacer(1, 30))
    summary_box = [
        [Paragraph(
            "<b>EXECUTIVE SUMMARY & PURPOSE:</b><br/>"
            "Coriolis is an open-source, 24/7 living room smart appliance combining a wall clock, "
            "ambient generative art, retro gaming, and a dedicated <b>glanceable guided fitness companion</b> "
            "(Yoga, Kettlebell/Exercise, Breathing, Pomodoro Focus). Developed desktop-first against an exact "
            "pixel simulator, the software runs deterministically on high-speed Teensy 4.1 hardware with "
            "zero heap allocations, 120 Hz refresh rates, 36-bit color depth, and completely offline local audio.",
            callout_style
        )]
    ]
    t_summary = Table(summary_box, colWidths=[485])
    t_summary.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), BG_LIGHT),
        ('BOX', (0,0), (-1,-1), 1, SECONDARY),
        ('PADDING', (0,0), (-1,-1), 12),
    ]))
    story.append(t_summary)

    story.append(Spacer(1, 40))
    story.append(Paragraph("<b>Contents Overview:</b>", body_bold))
    toc_text = (
        "1. Complete Bill of Materials (BOM) & Buying Guide<br/>"
        "2. Mechanical Construction & Enclosure Design<br/>"
        "3. Power Architecture & Electrical Safety<br/>"
        "4. Interconnect Schematics & Pinout Mapping<br/>"
        "5. Firmware Flashing & SD Card Audio Setup<br/>"
        "6. User Manual: Remote Controls, Fitness Suite & Navigation"
    )
    story.append(Paragraph(toc_text, body_style))

    story.append(PageBreak())

    # =========================================================================
    # SECTION 1: COMPLETE BILL OF MATERIALS (BOM)
    # =========================================================================
    story.append(Paragraph("1. Complete Bill of Materials (BOM) & Sourcing", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=PRIMARY, spaceAfter=8))
    story.append(Paragraph(
        "Every component listed below has been vetted for compatibility with the Coriolis architecture. "
        "Estimated prices are current as of late 2026.",
        body_style
    ))

    bom_headers = ["Item / Description", "Qty", "Est. Cost", "Recommended Supplier / Notes"]
    bom_data = [
        [Paragraph(h, table_header_style) for h in bom_headers],
        [
            Paragraph("<b>Teensy 4.1 Microcontroller</b><br/>600 MHz ARM Cortex-M7, 1MB RAM, RTC, USB Host", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€32", table_cell_style),
            Paragraph("PJRC.com or authorized EU distributor (Mouser, DigiKey, Pimoroni). Buy with pins or solder header.", table_cell_style)
        ],
        [
            Paragraph("<b>SmartLED Shield V5 for Teensy 4</b><br/>Level shifters, panel driver connector, RTC backup", table_cell_style),
            Paragraph("1 (or 2)", table_cell_style),
            Paragraph("~€24", table_cell_style),
            Paragraph("Crowd Supply or Smart-Prototyping. Buy 2 for a bench spare.", table_cell_style)
        ],
        [
            Paragraph("<b>64×64 HUB75 LED Panels (P4 or P5)</b><br/>P4 = 256×256mm each (512×512mm matrix)<br/>P5 = 320×320mm each (640×640mm matrix)", table_cell_style),
            Paragraph("4", table_cell_style),
            Paragraph("~€120-160", table_cell_style),
            Paragraph("AliExpress (Coreman, Qiangli, or LINSN store). Crucial: order all 4 together in one order to guarantee identical batch & 1/32 scan driver ICs.", table_cell_style)
        ],
        [
            Paragraph("<b>5V Power Supply (Mean Well LRS-200-5)</b><br/>5V DC, 40A, 200W industrial switching PSU", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€38", table_cell_style),
            Paragraph("Mouser, DigiKey, or Mean Well official store on Amazon. Never skimp on PSU; matrix peaks at 25-30A full white.", table_cell_style)
        ],
        [
            Paragraph("<b>Aluminum Unibody IR Remote (Apple A1294)</b><br/>or Universal 38kHz NEC Remote Control", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€12-18", table_cell_style),
            Paragraph("eBay or Amazon (search 'Apple Remote A1294 aluminum'). Premium tactile feel; firmware includes native decoder.", table_cell_style)
        ],
        [
            Paragraph("<b>TSOP38238 38kHz IR Receiver Sensor</b><br/>Active-low demodulating IR sensor with filter", table_cell_style),
            Paragraph("2", table_cell_style),
            Paragraph("~€2", table_cell_style),
            Paragraph("Adafruit (#157) or Mouser. Low power, high ambient noise immunity.", table_cell_style)
        ],
        [
            Paragraph("<b>DFPlayer Mini MP3 Serial Board</b><br/>HW-247A / YX5200 serial MP3 audio module", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€4", table_cell_style),
            Paragraph("AliExpress or Amazon. Provides 100% offline local speech and chimes.", table_cell_style)
        ],
        [
            Paragraph("<b>MicroSD Card (16GB or 32GB, Class 10)</b><br/>Formatted FAT32 for DFPlayer audio pack", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€6", table_cell_style),
            Paragraph("SanDisk Ultra or Kingston (under 32GB for FAT32 compatibility).", table_cell_style)
        ],
        [
            Paragraph("<b>3W 4Ω or 8Ω Enclosed Speaker</b><br/>Miniature chamber speaker with JST connector", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€4", table_cell_style),
            Paragraph("Adafruit (#4445 or #3885) or AliExpress. Plugs directly into DFPlayer SPK pins.", table_cell_style)
        ],
        [
            Paragraph("<b>CR2032 Coin Cell Battery & Holder</b><br/>Backs up the Teensy RTC across wall unplugs", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€2", table_cell_style),
            Paragraph("Local electronics shop or Amazon. Solder leads to VBAT and GND.", table_cell_style)
        ],
        [
            Paragraph("<b>2020 Aluminum Extrusion & Brackets</b><br/>For rigid square frame (approx 52cm or 65cm rails)", table_cell_style),
            Paragraph("4 rails", table_cell_style),
            Paragraph("~€25", table_cell_style),
            Paragraph("Motedis, Rat Rig, or Amazon. 2020 T-slot extrusion + corner angle brackets.", table_cell_style)
        ],
        [
            Paragraph("<b>Smoked Gray Tinted Acrylic Sheet (3mm)</b><br/>15-20% light transmission for black contrast", table_cell_style),
            Paragraph("1", table_cell_style),
            Paragraph("~€20-25", table_cell_style),
            Paragraph("Plastics online shop cut to size (515×515mm for P4, 645×645mm for P5). Vital for panel contrast.", table_cell_style)
        ],
        [
            Paragraph("<b>Wiring, Capacitors & Fasteners</b><br/>16/18 AWG silicone power wire, 4x 1000µF 10V caps, 1kΩ resistor, M3 screws for panel backs", table_cell_style),
            Paragraph("Kit", table_cell_style),
            Paragraph("~€15", table_cell_style),
            Paragraph("Electronics bench supplies / hardware store.", table_cell_style)
        ],
    ]

    t_bom = Table(bom_data, colWidths=[180, 45, 65, 195])
    t_bom.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), PRIMARY),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, BG_LIGHT]),
    ]))
    story.append(t_bom)

    story.append(Spacer(1, 8))
    total_box = [
        [Paragraph("<b>ESTIMATED TOTAL SYSTEM HARDWARE COST:</b>", body_bold), Paragraph("<b>~€280 — €330</b> (Complete)", body_bold)]
    ]
    t_total = Table(total_box, colWidths=[300, 185])
    t_total.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#E2ECE9")),
        ('BOX', (0,0), (-1,-1), 1, SECONDARY),
        ('PADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(t_total)

    story.append(PageBreak())

    # =========================================================================
    # SECTION 2: MECHANICAL ASSEMBLY & FRAME
    # =========================================================================
    story.append(Paragraph("2. Mechanical Construction & Enclosure Design", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=PRIMARY, spaceAfter=8))

    story.append(Paragraph(
        "The display consists of four identical 64×64 HUB75 panels arranged into a 2×2 square matrix. "
        "Rigidity and alignment precision are paramount: if the panels are misaligned by even 0.5mm, "
        "the seam will be visible when lit.",
        body_style
    ))

    story.append(Paragraph("Four-Panel Arrangement (128×128):", h2_style))
    story.append(Paragraph(
        "• <b>Top-Left:</b> Panel 1 (HUB75 Input from SmartLED Shield)<br/>"
        "• <b>Top-Right:</b> Panel 2 (HUB75 Chained from Panel 1 Output)<br/>"
        "• <b>Bottom-Right:</b> Panel 3 (HUB75 Chained from Panel 2 Output)<br/>"
        "• <b>Bottom-Left:</b> Panel 4 (HUB75 Chained from Panel 3 Output)<br/>"
        "<i>Note: SmartMatrix maps the panels into a single contiguous 128×128 buffer automatically.</i>",
        body_style
    ))

    story.append(Paragraph("Outer Frame & Diffusion Layer:", h2_style))
    story.append(Paragraph(
        "1. <b>Structural Chassis:</b> Assemble a square outer frame using 2020 aluminum extrusion and 90° cast corner brackets. "
        "For P4 panels (256mm edge), cut four 514mm rails. For P5 panels (320mm edge), cut four 642mm rails.<br/>"
        "2. <b>Panel Mounting:</b> Each HUB75 panel has M3 threaded brass inserts on its rear perimeter. "
        "Use 3D-printed alignment tabs or aluminum flat stock crossbars behind the panels to lock all four modules flush together.<br/>"
        "3. <b>Contrast Filter (Crucial):</b> Bare unlit LED matrix panels appear dark gray. "
        "Mount a 3mm smoked gray acrylic sheet (15–20% light transmission) directly in front of the LEDs with a 2–3mm air gap. "
        "This turns unlit pixels pitch black and dramatically amplifies apparent contrast and color saturation.",
        body_style
    ))

    story.append(Spacer(1, 10))

    # =========================================================================
    # SECTION 3: POWER ARCHITECTURE & ELECTRICAL SAFETY
    # =========================================================================
    story.append(Paragraph("3. Power Architecture & Electrical Safety", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=PRIMARY, spaceAfter=8))

    story.append(Paragraph(
        "A 128×128 matrix contains 16,384 RGB LEDs. At maximum white brightness, current draw can peak at ~25A. "
        "During normal Coriolis operation (ambient art, clocks, fitness figures), current draw averages 3A to 8A. "
        "Safe, solid wiring is essential:",
        body_style
    ))

    power_rules = [
        "<b>Dedicated 5V PSU:</b> Use a Mean Well LRS-200-5 (5V 40A). Ensure AC mains (L, N, PE) are wired into a fused IEC socket with safety earth bonded to the aluminum frame.",
        "<b>Star Topology Power Distribution:</b> Do NOT daisy-chain power from one panel to the next. Run four separate 16 AWG or 18 AWG copper wire pairs from the PSU terminal block directly to each individual panel's power screw terminals.",
        "<b>Local Decoupling Capacitors:</b> Solder a 1000 µF 6.3V or 10V low-ESR electrolytic capacitor across the 5V/GND terminals of each of the 4 panels. This absorbs high-frequency PWM switching spikes.",
        "<b>Common Grounding:</b> Connect the Teensy 4.1 GND, SmartLED Shield GND, DFPlayer GND, and IR Receiver GND directly to the main power supply negative rail."
    ]
    for r in power_rules:
        story.append(Paragraph(f"• {r}", body_style))

    story.append(PageBreak())

    # =========================================================================
    # SECTION 4: INTERCONNECT SCHEMATICS & PINOUT MAPPING
    # =========================================================================
    story.append(Paragraph("4. Hardware Schematics & Interconnect Mapping", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=PRIMARY, spaceAfter=8))

    story.append(Paragraph(
        "All peripherals interface directly with the Teensy 4.1 + SmartLED Shield V5 stack. "
        "The SmartLED Shield plugs onto the Teensy header pins, leaving peripheral pins accessible:",
        body_style
    ))

    story.append(Paragraph("Peripheral Pinout Table:", h2_style))

    pin_headers = ["Subsystem", "Module Pin", "Teensy 4.1 Pin", "Notes / Conditioning"]
    pin_data = [
        [Paragraph(h, table_header_style) for h in pin_headers],
        [
            Paragraph("<b>IR Receiver</b><br/>(TSOP38238)", table_cell_style),
            Paragraph("Pin 1: OUT<br/>Pin 2: GND<br/>Pin 3: VS (3.3V)", table_cell_style),
            Paragraph("<b>Pin 2</b> (Interrupt)<br/>GND<br/>3.3V", table_cell_style),
            Paragraph("Hardware interrupt line. Place 100Ω resistor and 4.7µF cap on Vs line for noise filtering.", table_cell_style)
        ],
        [
            Paragraph("<b>DFPlayer Mini</b><br/>(Audio MP3)", table_cell_style),
            Paragraph("Pin 1: VCC (5V)<br/>Pin 2: RX<br/>Pin 3: TX<br/>Pin 7: GND", table_cell_style),
            Paragraph("5V (Ext PSU)<br/><b>Pin 1</b> (Serial1 TX)<br/><b>Pin 0</b> (Serial1 RX)<br/>GND", table_cell_style),
            Paragraph("<b>Essential:</b> Put a 1kΩ resistor in series between Teensy Pin 1 (TX) and DFPlayer RX to suppress line hum.", table_cell_style)
        ],
        [
            Paragraph("<b>Speaker</b><br/>(3W 4Ω)", table_cell_style),
            Paragraph("Positive (+)<br/>Negative (-)", table_cell_style),
            Paragraph("DFPlayer Pin 6 (SPK1)<br/>DFPlayer Pin 8 (SPK2)", table_cell_style),
            Paragraph("Direct speaker differential output from DFPlayer on-board amplifier.", table_cell_style)
        ],
        [
            Paragraph("<b>RTC Backup</b><br/>(CR2032)", table_cell_style),
            Paragraph("Positive (+)<br/>Negative (-)", table_cell_style),
            Paragraph("<b>VBAT</b> pad<br/>GND", table_cell_style),
            Paragraph("Keeps RTC clock accurate when display is switched off.", table_cell_style)
        ],
        [
            Paragraph("<b>SmartLED Shield</b><br/>(HUB75 Ribbons)", table_cell_style),
            Paragraph("16-pin IDC ribbon", table_cell_style),
            Paragraph("Direct Shield Header", table_cell_style),
            Paragraph("Connects to Input IDC of Top-Left Panel. High-speed DMA driven.", table_cell_style)
        ],
    ]

    t_pins = Table(pin_data, colWidths=[110, 110, 120, 145])
    t_pins.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), PRIMARY),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, BG_LIGHT]),
    ]))
    story.append(t_pins)

    story.append(Spacer(1, 10))

    # =========================================================================
    # SECTION 5: FIRMWARE FLASHING & AUDIO PACK SETUP
    # =========================================================================
    story.append(Paragraph("5. Firmware Flashing & SD Card Audio Setup", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=PRIMARY, spaceAfter=8))

    story.append(Paragraph(
        "Coriolis is built and flashed using <code>arduino-cli</code>. The repository includes an automated setup script "
        "that patches SmartMatrix 4 for GCC 11 compatibility and junctions the shared code automatically:",
        body_style
    ))

    flash_steps = [
        "<b>1. Install Toolchain:</b> Open PowerShell as Administrator and run: <code>winget install ArduinoSA.CLI</code>",
        "<b>2. Run Setup Script:</b> Inside the repository root, run: <code>powershell -File firmware/setup.ps1</code>. This installs the Teensy core, SmartMatrix 4, and establishes <code>CoriolisShared</code>.",
        "<b>3. Compile & Upload:</b> Connect Teensy 4.1 via Micro-USB and execute:<br/>"
        "<code>arduino-cli compile --fqbn teensy:avr:teensy41 firmware/coriolis --upload</code>",
        "<b>4. Generate DFPlayer Voice Pack:</b> Run <code>python tools/generate_voice_pack.py</code>. "
        "This generates folder <code>01</code> (14 system cues & chimes) and folder <code>02</code> (54 voice clips: poses, moves, numbers 1..30).",
        "<b>5. Load MicroSD Card:</b> Format your microSD card (up to 32GB) as FAT32. Copy the generated folders <code>01</code> and <code>02</code> directly to the root of the card and insert into DFPlayer Mini."
    ]
    for s in flash_steps:
        story.append(Paragraph(s, body_style))

    story.append(PageBreak())

    # =========================================================================
    # SECTION 6: USER MANUAL & OPERATING GUIDE
    # =========================================================================
    story.append(Paragraph("6. Operating Guide & Feature Reference", h1_style))
    story.append(HRFlowable(width="100%", thickness=1, color=PRIMARY, spaceAfter=8))

    story.append(Paragraph(
        "Coriolis is operated via a minimalist infrared remote control (Apple A1294 aluminum remote or standard NEC remote). "
        "All commands are processed by the interrupt-driven decoder on Pin 2.",
        body_style
    ))

    story.append(Paragraph("Remote Control Mapping:", h2_style))
    remote_map = [
        ["Button", "Simulator Key", "Function / Context"],
        ["<b>Right (→)</b>", "<code>→</code> or <code>SPACE</code>", "Next scene (triggers smooth directional SlideLeft transition)"],
        ["<b>Left (←)</b>", "<code>←</code>", "Previous scene (triggers smooth directional SlideRight transition)"],
        ["<b>Menu</b>", "<code>M</code>", "Toggle Categorized Scene Launcher (instant quick-picker)"],
        ["<b>Center / OK</b>", "<code>ENTER</code>", "Start / Pause active routine or game. Select launcher item."],
        ["<b>Play / Pause</b>", "<code>ENTER</code>", "Instant workout pause/resume; mat-friendly control"],
        ["<b>Up (↑)</b>", "<code>↑</code>", "Adjust pace / reps during fitness; navigate menus; cycle palette"],
        ["<b>Down (↓)</b>", "<code>↓</code>", "Adjust pace / reps during fitness; navigate menus; cycle palette"],
        ["<b>Back (Hold)</b>", "<code>BACKSPACE</code>", "Home directly to Clock from anywhere; dismiss overlays"],
    ]
    t_remote = Table([[Paragraph(c, table_header_style if i == 0 else table_cell_style) for c in row] for i, row in enumerate(remote_map)], colWidths=[90, 110, 285])
    t_remote.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), PRIMARY),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, BG_LIGHT]),
    ]))
    story.append(t_remote)

    story.append(Spacer(1, 8))
    story.append(Paragraph("The Guided Fitness & Yoga Suite:", h2_style))
    story.append(Paragraph(
        "Engineered to convincingly replace YouTube fitness videos with a distraction-free, glanceable companion:<br/>"
        "• <b>Bilateral Flow Engine:</b> Poses requiring left/right execution automatically mirror with anatomical joint swapping, "
        "presenting clear <b>(R)</b> and <b>(L)</b> side banners and voice prompts.<br/>"
        "• <b>Next-Up HUD Previews:</b> 4 seconds before any posture or exercise changes, a high-visibility badge previews the next movement.<br/>"
        "• <b>5 Complete Yoga Flows:</b> <i>Morning Wake-Up</i>, <i>Sun Salutation</i>, <i>Standing Strength & Balance</i>, <i>Hips & Back</i>, and <i>Bedtime Restorative</i>.<br/>"
        "• <b>4 Complete Exercise Programs:</b> <i>Bodyweight Circuit</i>, <i>Kettlebell Full-Body</i>, <i>HIIT Tabata</i> (20s work / 10s rest with full-screen emerald/amber shifts), and <i>Core & Pilates</i>.<br/>"
        "• <b>Audio Guidance:</b> Chimes, start/finish bells, halfway alerts (50% elapsed hold chime), 3-2-1 ticks, and spoken move names.",
        body_style
    ))

    story.append(Spacer(1, 8))
    story.append(Paragraph("Settings & Night Mode Auto-Dimming:", h2_style))
    story.append(Paragraph(
        "Press <code>S</code> (or Remote Settings) to access the scrolling configuration menu:<br/>"
        "• <b>GENERAL:</b> Brightness (16..255), Palette, Transition Style (AUTO, SLIDE, FADE, CUT), Rotation (0°, 90°, 180°, 270°), Autoplay.<br/>"
        "• <b>NIGHT:</b> Auto-dimming schedule. Default: <b>23:00 to 07:00</b> at <b>16 brightness</b>. Drops display brightness to a soft sleep glow without needing user intervention.<br/>"
        "• <b>OVERLAY:</b> Clock overlay (Digital, Analog, Word) pinned over any animation scene.",
        body_style
    ))

    story.append(Spacer(1, 15))
    concl_box = [
        [Paragraph(
            "<b>BUILDER NOTE:</b> All software features, bilateral posture animations, transition compositing, "
            "and IR remote decoders have been thoroughly tested and verified against the desktop simulator and Teensy 4.1 hardware toolchain. "
            "When you begin physical assembly in your new home, your software is already 100% complete and fully perfected.",
            callout_style
        )]
    ]
    t_concl = Table(concl_box, colWidths=[485])
    t_concl.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), BG_LIGHT),
        ('BOX', (0,0), (-1,-1), 1, ACCENT),
        ('PADDING', (0,0), (-1,-1), 10),
    ]))
    story.append(t_concl)

    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"Manual built successfully: {filename}")


if __name__ == "__main__":
    build_pdf()
