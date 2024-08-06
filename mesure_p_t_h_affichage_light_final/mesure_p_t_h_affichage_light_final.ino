#include "LCD_perso.h"
#include "ruche_fonction.h"
#include "HX711_perso.h"
#include <avr/sleep.h>
#include <avr/wdt.h>


//configuration du branchement du capteur de température et d'humidité
#define ALIM_AM2302 5   //broche d'alimentation du capteur AM2302
#define BROCHE_CAPTEUR 6    // La ligne de communication du DHT22 (AM2302 sera donc branchée sur la pin D6 de l'Arduino

//configuration du branchement de l'afficheur LCD NOKIA5110
#define RST 9           //broche PB1 patte 15 sur l'atmel reset de l'afficheur SPI
#define CE  10          //broche PB2 patte 16 sur l'atmel chip enable de l'afficheur SPI
#define DO  12          //broche PB4 patte 18 sur l'atmel miso de l'afficheur SPI
#define CLK 13          //broche PB5 patte 19 sur l'atmel
#define DIN 11          //broche PB3 patte 17 sur l'atmel

//configuration du convertisseur pour la mesure de la masse
#define DT  7           //ligne data du convertisseur pour cellule de charge HX711 (broche PD6 patte 12 sur l'atmel)
#define SCK 8           //ligne d'horloge pour le convertisseur HX711 (broche PD7 patte 13 sur l'atmel)

//configuration des deux entrees d'interruptions
#define RINT0 2          //entree d'interruption patte 4 de l'atmel (mesure instantannee du poids bouton rouge)
#define RINT1 3          //entree d'interruption patte 5 de l'atmel (mise en mémoire bouton bleu)

//Variables partagée par les trois routines d'interruptions et le programme principal
volatile bool routineWDT = false;  //passage par l'interruption watchdog
volatile bool routineINT0 = false; //passage par l'interruption du bouton rouge
volatile bool routineINT1 = false; //passage par l'interruption du bouton bleu
volatile unsigned int compteur=0;  //compteur pour compter toutes les heures 450 passages
                                   //de 8 secondes pour arriver à 1 heures

//routine d'interruption du chien de garde (sert juste à réveiller le uC) et incrémenter le compteur
ISR (WDT_vect) {
  __asm__ __volatile__("wdr");               //reset du timer watchdog 
  routineWDT=true;
  compteur++;         
}

//action sur le bouton rouge relié en patte 4 de l'atmel D2
ISR(INT0_vect) {                             //mesure instantannee du poids
  EIMSK=0x00;                                //on desactive l'interruption sur INT1 et INT0
  __asm__ __volatile__("wdr");               //reset du timer watchdog
  routineINT0 = true;
}

//action sur le bouton bleu relié en patte 5 de l'atmel D3
ISR(INT1_vect) {                             //mise en mémoire du poids dans memPoids
  EIMSK=0x00;                                //on desactive l'interruption sur INT1 et INT0
  __asm__ __volatile__("wdr");               //reset du timer watchdog                  
  routineINT1 = true;
} 

int main (void) {
  init();
// Initialisation du mode sommeil
  initSleep8S();
 
//Initialisation du système
  // Variable qui permet de stocker l'ensemble des données de mesure 
  InfoRuche infoRuche;
  bool firstStart=true;
  bool mesureIm=false;
  bool uneHeure=false;
  pinMode(RINT0, INPUT_PULLUP);   //interupteur sur broche D2 INT0 : mesure instantannée du poids
  pinMode(RINT1, INPUT_PULLUP);   //interupteur sur broche D3 INT1 : mise en mémoire 
  
  
  // Fixation de l'identifiant de la ruche
  infoRuche.identifiant="12345";
  infoRuche.memPoids="000";

// Initialisation des capteurs de poids
  HX711 balance;
  //utilisation d'un gain de 128 sur le channel A (le 32 reservé pour le canal B)
  balance.begin(DT,SCK,128);

  //tare + facteur de convertion
  while (!balance.is_ready());                              //attention !! fonction bloquante tant que Dout n'est pas au niveau bas
  balance.set_scale(2150.f);                                //valeur trouvee après la procedure de calibration
  balance.tare(10);                                         //permet de soustraire la valeur au démarrage pour afficher 0 (tarage de la balance) avec 10 mesures

  
  // Initialisation du capteur AM2302 patte en entrée pour éviter les lectures 
  pinMode(BROCHE_CAPTEUR, INPUT_PULLUP);

// Gestion de l'afficheur
  
  static PCD8544 lcd(CLK, DIN, DO, RST, CE);

  // Symbol du degrès celsius
  static const byte degre[] = { 0x00, 0x00, 0x02, 0x05, 0x02};

  // Lancement de l'afficheur
  lcd.begin(84,48);
  lcd.setContrast(70);
  lcd.createChar(0,degre);
  
  // Affichage de l'interface
  lcd.setCursor(10,0);
  lcd.print(F("Temp"));
  lcd.setCursor(30,1);
  lcd.write(0);
  lcd.setCursor(36,1);
  lcd.print(F("C"));
  lcd.setCursor(55,0);
  lcd.print(F("Hum"));
  lcd.setCursor(70,1);
  lcd.print(F("%"));
  lcd.setCursor(0,3);
  lcd.print(F("Poids"));
  lcd.setCursor(55,3);
  lcd.print(F("mem :"));
  lcd.setCursor(27,4);
  lcd.print(F("Kg"));
  lcd.setCursor(73,4);
  lcd.print(F("Kg"));
 
/*******************************************************************************************************************/
// les interruptions sur INT0 et INT1 sur niveau pour sortir du mode sommeil                                       //
/*******************************************************************************************************************/
  EICRA&=~(1<<ISC11);
  EICRA&=~(1<<ISC10);         //interruption de INT1 sur niveau bas (obligatoire pour sortir de sleep);
  EICRA&=~(1<<ISC01);
  EICRA&=~(1<<ISC00);         //interruption de INT0 sur niveau bas (obligatoire pour sortir de sleep);

  // =================
  // Boucle principale
  // =================
  while (1) {
//si le réveil est du à l'appuie sur le bouton rouge (mesure immédiate du poids)
    if(routineINT0){
      routineINT0=false;      //on réinitialise le drapeau
      mesureIm=true;          //on effectuera une mesure
    }

//si le réveil est du à l'appuie sur le bouton bleu (mise en mémoire)
    if(routineINT1){
      routineINT1=false;                    //on réinitialise de drapeau
      infoRuche.memPoids=infoRuche.poids;   //on transfère la valeur du poids en mémoire
      affichage(&infoRuche,&lcd);           //on affiche la valeur 
      if (!digitalRead(RINT1)){
        while(!digitalRead(RINT1));          //on attend que l'utilisateur lache le bouton pour ne pas
      }                                     //repartir en interruption
      delay(20);                            //evite l'antirebond qui ferait repartir en interruption    
    }

//si le réveil est du au chien de garde
    if(routineWDT){                   //on compte le nombre de passage dans la boucle
      routineWDT=false;  
      if(compteur==450){                //ici on modifiera uneHeure au bout de 450*8 secondes
        uneHeure=true;
        compteur=0;
      }
    }

//si premier démarrage ou demande de mesure immédiate ou attente d'une heure, on effectue une lecture
    if(firstStart==true || uneHeure==true ||mesureIm==true){
          //reveil du HX711
          balance.power_up();
          // Lecture des données 
          // On ne peut pas lire le capteur AM2302 plus vite qu'une fois toutes les 2 secondes !!
          // le capteur est alimenté par la patte 13 (PD7) de l'atmel
          pinMode(ALIM_AM2302,OUTPUT);        
          digitalWrite(ALIM_AM2302,HIGH);
          delay(30);                      //etablissement de la tension pour le capteur
          mesureTH(BROCHE_CAPTEUR,&infoRuche);
          digitalWrite(ALIM_AM2302,LOW);
          pinMode(ALIM_AM2302,INPUT);             //on gagne 20uA en commutant la broche en entrée
          infoRuche.poids=balance.get_units(10);  //lecture du poids
          balance.power_down();                   //mise en mode sommeil du convertisseur HX711
          affichage(&infoRuche, &lcd);            //afichage de infos sur le lcd
          firstStart=false;                       //juste executé au démarrage
          mesureIm=false;                         //si il s'agit de la mesure immédiate
          uneHeure=false;                         //si il s'agit d'un passage au bout d'une heure
          if(!digitalRead(RINT0)){                //si le passage dans la boucle est du à l'appuie sur
            while(!digitalRead(RINT0));           //le bouton rouge INT0, on attend le relachement pour ne
            delay(20);                            //pas repartir en interruption (avec l'antirebond ici )
          }                 
    }
          
/****************************************************************************************************/
//activation du sommeil de l'atmel                                                                  //
/****************************************************************************************************/

  __asm__ __volatile__("wdr");   //reset du timer watchdog
  //désactivation des interruptions
  __asm__ __volatile__ ("cli");  //commande assembleur pour mettre à 0  le bit I de SREG

  ;
  //désactivation du BOD (mesure de la jute de tension d'alimentation) page 38 de la doc
  MCUCR = (MCUCR | (1 << BODS)) | (1 << BODSE); //procédure pour désactiver le BOD
  MCUCR = (MCUCR & ~(1 << BODSE)) | (1 << BODS);

  // Extinction du convertisseur 
  ADCSRA &= ~(1 << ADEN); //page 218 de la doc
 
  //activation des interruptions
  EIMSK|=1<<INT1;             //autorisation d'une interruption sur INT1
  EIMSK|=1<<INT0;             //autorisation d'une interruption sur INT0
  __asm__ __volatile__("sei");
  __asm__ __volatile__("sleep"); //capteur AM2302 sur alimentation broche plus afficheur allumé constament
                                 //donne 254uA
                                 //donne 282uA avec la balance connectée et le mode sommeil activée du HX711
                                 //donne entre 278uA et 290uA avec les interruptions en pullup sur INT1 et INT0 
  //le programme reprend ici après le réveil par une interruption et l'execution de la routine 
  //d'interruption (watchdog, INT0 ou INT1)                               
  }
  return 0;
}
