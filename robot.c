#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <softPwm.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define RANGE 100

// Definimos los pins de control
#define CHAN_CONFIG_SINGLE  8
#define RUEDA_DER 1 // PIN 18
#define RUEDA_IZQ 0 // PIN 17

// Definimos los canales de los sensores
#define CH_OBS_DER 0
#define CH_OBS_IZQ 1
#define CH_LIN_DER 2
#define CH_LIN_IZQ 3

// Definimos el movimiento de la rueba
#define MOV_HOR 14.9 //Hacia horario
#define MOV_AHOR 17 //Hacia antihorario
#define MOV_STOP 0 // Parado
static int myFd ;

void loadSpiDriver()
{
    if (system("gpio load spi") == -1)
    {
        fprintf (stderr, "Can't load the SPI driver: %s\n", strerror (errno)) ;
        exit(EXIT_FAILURE) ;
    }
}

void spiSetup (int spiChannel)
{
    if ((myFd = wiringPiSPISetup (spiChannel, 1000000)) < 0)
    {
        fprintf (stderr, "Can't open the SPI bus: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }
}

int myAnalogRead(int spiChannel,int channelConfig,int analogChannel)
{
    if(analogChannel<0 || analogChannel>7)
        return -1;
    unsigned char buffer[3] = {1}; // start bit
    buffer[1] = (channelConfig+analogChannel) << 4;
    wiringPiSPIDataRW(spiChannel, buffer, 3);
    return ( (buffer[1] & 3 ) << 8 ) + buffer[2]; // get last 10 bits
}

// INICIALIZACION
int init(){
printf("Iniciando programa...\n");
  wiringPiSetup();
  spiSetup(0); 
  softPwmCreate (RUEDA_DER, 0, RANGE);
  softPwmCreate (RUEDA_IZQ, 0, RANGE);
  return 0;
}

int main ()
{ 
  int spiChannel=0;
  int channelConfig=CHAN_CONFIG_SINGLE;
  int obstaculo = 0;
  int linea = 0; // 0 = BIEN, 1 = Se sale DER, 2=Se sale IZQ
  int dis_obstaculo = 200;
  int lin_negra = 300;
  int lee_obs_der = 0;
  int lee_obs_izq = 0;
  int lee_lin_der = 0;
  int lee_lin_izq = 0;
  init();
  while(1){
    // Leemos sensores
    lee_obs_der = myAnalogRead(spiChannel,channelConfig,CH_OBS_DER);
    lee_obs_izq = myAnalogRead(spiChannel,channelConfig,CH_OBS_IZQ);
    lee_lin_der = myAnalogRead(spiChannel,channelConfig,CH_LIN_DER);
    lee_lin_izq = myAnalogRead(spiChannel,channelConfig,CH_LIN_IZQ);
    
    // Analizamos obstaculos y linea
    if( lee_obs_der > dis_obstaculo || lee_obs_izq > dis_obstaculo)
        obstaculo = 1; //Detectado obstaculo
    //Analizamos la linea
    if (lee_lin_izq > lin_negra && lee_lin_der > lin_negra)
        linea = 0;
    else if( lee_lin_izq < lin_negra && lee_lin_der > lin_negra)
        linea = 1; // Se sale por la der
    else if( lee_lin_der < lin_negra && lee_lin_izq > lin_negra)
        linea = 2; //Se sale por la izq
    obstaculo = 0;
    // Movemos ruedas segun la lectura obtenida
    if (obstaculo == 1 ){ //Si encuentro un obstaculo paro
	softPwmWrite(RUEDA_DER, MOV_STOP);
        softPwmWrite(RUEDA_IZQ, MOV_STOP);
    }
    else{// no hay obstaculo	
    	if (linea == 0){ // Seguimos recto
            softPwmWrite(RUEDA_DER, MOV_HOR);
	    softPwmWrite(RUEDA_IZQ, MOV_AHOR);
        }
        else if (linea == 1) // Me estoy saliendo por la derecha
            softPwmWrite(RUEDA_DER, MOV_STOP);
        else if (linea == 2) // Me estoy saliendo por la izquierda
            softPwmWrite(RUEDA_IZQ, MOV_STOP); 
    }
    if(obstaculo == 1) 
        printf("Obstaculo");
    if(linea == 1 || linea == 2){
        printf("Linea izquierda: %i\n",lee_lin_der);
        printf("Linea derecha: %i\n",lee_lin_izq);
    }
    obstaculo = 0;
  }
  return 0;
}

