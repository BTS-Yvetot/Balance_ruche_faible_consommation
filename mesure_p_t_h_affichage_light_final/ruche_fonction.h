#ifndef RUCHEFONCTION_H
#define RUCHEFONCTION_H
#include "Arduino.h"
#include <SPI.h>                //utilisée pour l'afficheur NOKIA5110
#include "LCD_perso.h"

/* Code d'erreur de la fonction mesureTH() */
const byte DHT_SUCCESS = 0;        // Pas d'erreur
const byte DHT_TIMEOUT_ERROR = 1;  // Temps d'attente dépassé
const byte DHT_CHECKSUM_ERROR = 2; // Données reçues erronées


typedef struct InfoRuche{
  String          identifiant;
  String          humidite;
  String          temperature;
  String          poids;
  String          memPoids;
}InfoRuche;

void initSleep8S();                                                       //configure le mode sommeil et le watchdog pour un temps de 8secondes et 
                                                                          //un sommeil profond
byte readAM2302(byte BROCHE_CAPTEUR,byte* data, const unsigned long start_time, unsigned long timeout); // 
                                                                          //extraction des données brutes du capteur
byte mesureTH (const byte BROCHE_CAPTEUR, InfoRuche* const infoRuche);    //effectue les mesures de T° et d'humidité et mets à jour data[5]
String getHumidity(byte* const data);                                     //extrait la mesure d'humidité de data[5] et met à jour la structure
String getTemperature(byte* const data);                                  //extrait la mesure de temperature de date[5] et met à jour la structure
void affichage (const InfoRuche* const infoRuche, PCD8544* lcd);          //affiche les données variables T° Humidité et poids
String convertionStoT(String temperature);                                //ajoute une virgule sur la mesure de temperature en dixième de degrès
String convertionStoP(String poids);                                      //ajoute une virgule sur la mesure du poids en dixième de kilo
#endif
