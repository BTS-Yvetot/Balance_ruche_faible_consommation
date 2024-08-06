#ifndef LCD_PERSO_H
#define LCD_PERSO_H
#include <Arduino.h>
#define CHIP_PCD8544 0

class PCD8544: public Print {       //permet d'utiliser la fonction print() avec différents formats
    public:

        // Attention : précisez absolument dans le constructeur les bonnes broches
        PCD8544(uint8_t sclk  = 3,   /* clock       (display pin 2) */
                uint8_t sdin  = 4,   /* data-in     (display pin 3) */
                uint8_t dc    = 5,   /* data select (display pin 4) */
                uint8_t reset = 6,   /* reset       (display pin 8) */
                uint8_t sce   = 7);  /* enable      (display pin 5) */

        // Démarrage de l'afficheur 84x48
        void begin(uint8_t width=84, uint8_t height=48, uint8_t model=CHIP_PCD8544);
        void stop();

        // effacer l'écran
        void clear();
        void clearLine();  // juste la ligne en cours

        // Gestion de la puissance (mode sommeil)
        void setPower(bool on);

        // mode d'affichage
        void setInverse(bool enabled);        // tout l'écran
        void setInverseOutput(bool enabled);  // uniquement la suite

        // constrast de 0 à 127
        void setContrast(uint8_t level);


        // Positionnement du curseur
        void setCursor(uint8_t column, uint8_t line);

        // place un symbol (5x8) dans un tableau de (0-31)...
        void createChar(uint8_t chr, const uint8_t *glyph);

        // permet d'afficher le symbol du tableau
        virtual size_t write(uint8_t chr);

    private:
        uint8_t pin_sclk;
        uint8_t pin_sdin;
        uint8_t pin_dc;
        uint8_t pin_reset;
        uint8_t pin_sce;

        // Chip variant in use...
        uint8_t model;

        // The size of the display, in pixels...
        uint8_t width;
        uint8_t height;

        // Current cursor position...
        uint8_t column;
        uint8_t line;

        // Current output mode for writes (doesn't apply to draws)...
        bool inverse_output = false;

        // User-defined glyphs (below the ASCII space character)...
        const uint8_t *custom[' '];

        // Send a command or data to the display...
        void send(uint8_t type, uint8_t data);
};


#endif  /* PCD8544_H */


/* vim: set expandtab ts=4 sw=4: */
