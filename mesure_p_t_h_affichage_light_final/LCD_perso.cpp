#include "LCD_perso.h"

#include <Arduino.h>

#if defined (__XTENSA__)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>       //utilisation de PROGMEM
#endif


#define PCD8544_CMD  LOW
#define PCD8544_DATA HIGH
#include "charset.cpp"          //a placer dans le même repertoire que LCD_perso.h


PCD8544::PCD8544(uint8_t sclk, uint8_t sdin, uint8_t dc, uint8_t reset, uint8_t sce):
    pin_sclk(sclk),
    pin_sdin(sdin),
    pin_dc(dc),
    pin_reset(reset),
    pin_sce(sce)
{}


void PCD8544::begin(uint8_t width, uint8_t height, uint8_t model)
{
    this->width = width;
    this->height = height;

    this->column = 0;
    this->line = 0;

    // symboles mis en mémoire 
    memset(this->custom, 0, sizeof(this->custom));

    
    pinMode(this->pin_sclk, OUTPUT);
    pinMode(this->pin_sdin, OUTPUT);
    pinMode(this->pin_dc, OUTPUT);
    pinMode(this->pin_reset, OUTPUT);
    pinMode(this->pin_sce, OUTPUT);

    // Reset de l'afficheur
    digitalWrite(this->pin_reset, HIGH);
    digitalWrite(this->pin_sce, HIGH);
    digitalWrite(this->pin_reset, LOW);
    delay(100);
    digitalWrite(this->pin_reset, HIGH);

    // Configuration de l'afficheur
    this->send(PCD8544_CMD, 0x21);  // extended instruction set control (H=1)
    this->send(PCD8544_CMD, 0x13);  // bias system (1:48)
    this->send(PCD8544_CMD, 0xc2);  // default Vop (3.06 + 66 * 0.06 = 7V)


    this->send(PCD8544_CMD, 0x20);  // extended instruction set control (H=0)
    this->send(PCD8544_CMD, 0x09);  // all display segments on

    // Clear RAM contents...
    this->clear();

    // Activate LCD...
    this->send(PCD8544_CMD, 0x08);  // display blank
    this->send(PCD8544_CMD, 0x0c);  // normal mode (0x0d = inverse mode)
    delay(100);

    // Place the cursor at the origin...
    this->send(PCD8544_CMD, 0x80);
    this->send(PCD8544_CMD, 0x40);
}


void PCD8544::stop()
{
    this->clear();
    this->setPower(false);
}


void PCD8544::clear()
{
    this->setCursor(0, 0);

    for (uint16_t i = 0; i < this->width * (this->height/8); i++) {
        this->send(PCD8544_DATA, 0x00);
    }

    this->setCursor(0, 0);
}


void PCD8544::clearLine()
{
    this->setCursor(0, this->line);

    for (uint8_t i = 0; i < this->width; i++) {
        this->send(PCD8544_DATA, 0x00);
    }

    this->setCursor(0, this->line);
}


void PCD8544::setPower(bool on)
{
    this->send(PCD8544_CMD, on ? 0x20 : 0x24);
}

void PCD8544::setInverse(bool enabled)
{
    this->send(PCD8544_CMD, enabled ? 0x0d : 0x0c);
}


void PCD8544::setInverseOutput(bool enabled)
{
    this->inverse_output = enabled;
}


void PCD8544::setContrast(uint8_t level)
{
    // The PCD8544 datasheet specifies a maximum Vop of 8.5V for safe
    // operation in low temperatures, which limits the contrast level.
    if (this->model == CHIP_PCD8544 && level > 90) {
        level = 90;  // Vop = 3.06 + 90 * 0.06 = 8.46V
    }

    this->send(PCD8544_CMD, 0x21);  // extended instruction set control (H=1)
    this->send(PCD8544_CMD, 0x80 | (level & 0x7f));
    this->send(PCD8544_CMD, 0x20);  // extended instruction set control (H=0)
}


void PCD8544::setCursor(uint8_t column, uint8_t line)
{
    this->column = (column % this->width);
    this->line = (line % (this->height/9 + 1));

    this->send(PCD8544_CMD, 0x80 | this->column);
    this->send(PCD8544_CMD, 0x40 | this->line);
}


void PCD8544::createChar(uint8_t chr, const uint8_t *glyph)
{
    // ASCII 0-31 only...
    if (chr >= ' ') {
        return;
    }

    this->custom[chr] = glyph;
}


size_t PCD8544::write(uint8_t chr)
{
    // ASCII 7-bit only...
    if (chr >= 0x80) {
        return 0;
    }

    const uint8_t *glyph;
    uint8_t pgm_buffer[5];

    if (chr >= ' ') {
        // Regular ASCII characters are kept in flash to save RAM...
        memcpy_P(pgm_buffer, &charset[chr - ' '], sizeof(pgm_buffer));
        glyph = pgm_buffer;
    } else {
        // Custom glyphs, on the other hand, are stored in RAM...
        if (this->custom[chr]) {
            glyph = this->custom[chr];
        } else {
            // Default to a space character if unset...
            memcpy_P(pgm_buffer, &charset[0], sizeof(pgm_buffer));
            glyph = pgm_buffer;
        }
    }

    // Output one column at a time...
    for (uint8_t i = 0; i < 5; i++) {
        this->send(PCD8544_DATA, this->inverse_output ? ~glyph[i] : glyph[i]);
    }

    // One column between characters...
    this->send(PCD8544_DATA, this->inverse_output ? 0xff : 0x00);

    // Update the cursor position...
    this->column = (this->column + 6) % this->width;

    if (this->column == 0) {
        this->line = (this->line + 1) % (this->height/9 + 1);
    }

    return 1;
}


void PCD8544::send(uint8_t type, uint8_t data)
{
    digitalWrite(this->pin_dc, type);

    digitalWrite(this->pin_sce, LOW);
    shiftOut(this->pin_sdin, this->pin_sclk, MSBFIRST, data);
    digitalWrite(this->pin_sce, HIGH);
}


/* vim: set expandtab ts=4 sw=4: */
