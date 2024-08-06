#include "ruche_fonction.h"

byte mesureTH(const byte BROCHE_CAPTEUR, InfoRuche* const infoRuche){
  byte data[5];
  byte ret = readAM2302(BROCHE_CAPTEUR, data, 1, 1000);
  
  /* Détecte et retourne les erreurs de communication */
  if (ret != DHT_SUCCESS){ 
    return ret;
  }
  infoRuche->humidite=getHumidity(data);
  infoRuche->temperature=getTemperature(data);
  return DHT_SUCCESS; 
}

String getHumidity(byte* const data) {
  uint16_t hum;
  String humidity="";
  hum = (((uint16_t)data[0]) << 8) | data[1];
  hum *= 0.1;
  if (hum>=10){
      humidity=String(hum);
  }
  else if (hum<1){
    humidity="00";
  }
  else {
    humidity="0"+(String(hum));
  }
  return humidity;
}

String getTemperature(byte* const data) {
  int16_t temp;
  String temperature="";
  temp = (((int16_t)(data[2] & 0x7F)) << 8 )| data[3];
  if (data[2] & 0x80) {         //valeur negative de la temperature
    temp *= -1;
  }
  if (temp==0) {
    temperature=String("+000");
  }
  if (temp > 0 && temp <10){
    temperature ="+00"+(String(temp));
  }
  if (temp >=10 && temp < 100){
    temperature="+0"+(String(temp));
  }
  if (temp >= 100){
    temperature="+"+(String(temp));
  }
  if (temp < 0 && temp >-10 ){
    temp=abs(temp);
    temperature="-00"+(String(temp));
  }
  if (temp <= -10 && temp > -100){
    temp=abs(temp);
    temperature="-0"+(String(temp));
  }
  if (temp <= -100){
    temp=abs(temp);
    temperature="-"+(String(temp));
  }
  return temperature;
}

byte readAM2302(byte BROCHE_CAPTEUR,byte* data, const unsigned long start_time,unsigned long timeout){
   data[0] = data[1] = data[2] = data[3] = data[4] = 0; //initialisation du tableau  
  // start_time est en millisecondes
  // timeout est en microsecondes
  /* Conversion du numéro de broche Arduino en ports / masque binaire "bas niveau" */
  uint8_t bit = digitalPinToBitMask(BROCHE_CAPTEUR);
  uint8_t port = digitalPinToPort(BROCHE_CAPTEUR);
  volatile uint8_t *ddr = portModeRegister(port);   // Registre MODE (INPUT / OUTPUT)
  volatile uint8_t *out = portOutputRegister(port); // Registre OUT (écriture)
  volatile uint8_t *in = portInputRegister(port);   // Registre IN (lecture)
  
  /* Conversion du temps de timeout en nombre de cycles processeur */
  unsigned long max_cycles = microsecondsToClockCycles(timeout);
  
  /* Evite les problèmes de pull-up */
  *out |= bit;  // PULLUP
  *ddr &= ~bit; // INPUT
  delay(100);   // Laisse le temps à la résistance de pullup de mettre la ligne de données à HIGH
 
  /* Réveil du capteur */
  *ddr |= bit;  // OUTPUT
  *out &= ~bit; // LOW
  delay(start_time); // Temps d'attente à LOW causant le réveil du capteur
  // N.B. Il est impossible d'utilise delayMicroseconds() ici car un délai
  // de plus de 16 millisecondes ne donne pas un timing assez précis.
  
  /* Portion de code critique - pas d'interruptions possibles */
  noInterrupts();
  
  /* Passage en écoute */
  *out |= bit;  // PULLUP
  delayMicroseconds(40);
  *ddr &= ~bit; // INPUT
 
  /* Attente de la réponse du capteur */
  timeout = 0;
  while(!(*in & bit)) { /* Attente d'un état LOW */
    if (++timeout == max_cycles) {
        interrupts();
        return DHT_TIMEOUT_ERROR;
      }
  }
    
  timeout = 0;
  while(*in & bit) { /* Attente d'un état HIGH */
    if (++timeout == max_cycles) {
        interrupts();
        return DHT_TIMEOUT_ERROR;
      }
  }

  /* Lecture des données du capteur (40 bits) */
  for (byte i = 0; i < 40; ++i) {
 
    /* Attente d'un état LOW */
    unsigned long cycles_low = 0;
    while(!(*in & bit)) {
      if (++cycles_low == max_cycles) {
        interrupts();
        return DHT_TIMEOUT_ERROR;
      }
    }

    /* Attente d'un état HIGH */
    unsigned long cycles_high = 0;
    while(*in & bit) {
      if (++cycles_high == max_cycles) {
        interrupts();
        return DHT_TIMEOUT_ERROR;
      }
    }
    
    /* Si le temps haut est supérieur au temps bas c'est un "1", sinon c'est un "0" */
    data[i / 8] <<= 1;
    if (cycles_high > cycles_low) {
      data[i / 8] |= 1;
    }
  }
  
  /* Fin de la portion de code critique */
  interrupts();
 
  /*
   * Format des données :
   * [1, 0] = humidité en %
   * [3, 2] = température en degrés Celsius
   * [4] = checksum (humidité + température)
   */
   
  /* Vérifie la checksum */
  byte checksum = (data[0] + data[1] + data[2] + data[3]) & 0xff;
  if (data[4] != checksum)
    return DHT_CHECKSUM_ERROR; /* Erreur de checksum */
  else
    return DHT_SUCCESS; /* Pas d'erreur */
}



void affichage(const InfoRuche* const infoRuche, PCD8544* lcd){

    
    //affichage humidite
    lcd->setCursor(54,1);
    lcd->print(infoRuche->humidite);
    //affichage temperature
    lcd->setCursor(0,1);
    lcd->print(convertionStoT(infoRuche->temperature));

    //affichage poids
    lcd->setCursor(0,4);
    lcd->print(convertionStoP(infoRuche->poids));

    //affichage mem
    lcd->setCursor(46,4);
    lcd->print(convertionStoP(infoRuche->memPoids));
}

String convertionStoT(String temperature){    
  String tAffichage=temperature.substring(0,3)+","+temperature.substring(3,4); //insertion de la virgule pour avoir la valeur en degrès
  return tAffichage;
}

String convertionStoP(String poids){
  String tPoids=poids.substring(0,2)+","+poids.substring(2,3); //insertion de la virgule pour avoir la valeur en kilo
  return tPoids;
}

void initSleep8S(){
     //choix du mode de sommeil POWER_DOWN SM2=0 SM1=1 SM0=0 , page 37 de la doc
  SMCR |= (1 << SM1);
  SMCR &=~(1 << SM0);
  SMCR &=~(1 << SM2);
  
  //autorisation du mode sommeil
  SMCR |= (1 << SE);

// Extinction du convertisseur 

  //désactivation du convertisseur analogique numérique
  ADCSRA &= ~(1 << ADEN); //page 218 de la doc

// Initialisation du chien de garde à 8s en interruption
  // *******************************************************************************
  //  BITS DE CONFIGURATION DU WATCHDOG
  //                           (cf. page 47 du datasheet de l'ATmega328P)
  // *******************************************************************************
  //  Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0
  //  WDIF  | WDIE  | WDP3  | WDCE  |  WDE  | WDP2  | WDP1  | WDP0
  // *******************************************************************************

  __asm__ __volatile__("cli");            //interdire les interruptions
  __asm__ __volatile__("wdr");            //mise à 0 du compteur chien de garde
  
     

  //séquence pour autoriser les changement sur le watchdog
  WDTCSR|=(1<<WDCE)|(1<<WDE); //séquence expliquée p46 dans l'exemple
  
  //réglage du facteur de division à 1024 WDP3=1 WDP2=0 WDP1=0 WDP0=1 du registre WDTCSR et de WDIE
  //WDE à mis à 0 en même temps pour être en mode Interruption
  WDTCSR=0b01100001; 

  
  __asm__ __volatile__("sei"); //nécessaire pour utiliser delay() et autre joyeuseté !!!!
}
