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

// Definimos los valores para cada caso en el que puede detectar la linea
#define LIN_BB 0
#define LIN_BN 1
#define LIN_NB 2
#define LIN_NN 3

// Definimos el movimiento de la rueba
#define MOV_HOR 14.5 //Hacia horario
#define MOV_AHOR 16 //Hacia antihorario
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
  int lin_negra = 200;// 0 == negro , 1024 == blanco
  int lee_obs_der = 0;
  int lee_obs_izq = 0;
  int lee_lin_der = 0;
  int lee_lin_izq = 0;
  int i =0;
  init();
  while(1){
    // Leemos sensores
    lee_obs_der = myAnalogRead(spiChannel,channelConfig,CH_OBS_DER);
    lee_obs_izq = myAnalogRead(spiChannel,channelConfig,CH_OBS_IZQ);
    lee_lin_der = myAnalogRead(spiChannel,channelConfig,CH_LIN_DER);
    lee_lin_izq = myAnalogRead(spiChannel,channelConfig,CH_LIN_IZQ);
    linea=0;    
    // Analizamos obstaculos y linea
    if( lee_obs_der > dis_obstaculo || lee_obs_izq > dis_obstaculo)
        obstaculo = 1; //Detectado obstaculo
    //Analizamos la linea
    //Doble Blanco
//printf("%d----%d\n", lee_obs_izq, lee_lin_der);
    if (lee_lin_izq > lin_negra && lee_lin_der > lin_negra)
        linea = LIN_BB;
    //Blanco-Negro
    else if( lee_lin_izq < lin_negra && lee_lin_der > lin_negra)
        linea = LIN_NB; // Se sale por la der
    //Negro-Blanco
    else if( lee_lin_der < lin_negra && lee_lin_izq > lin_negra)
        linea = LIN_BN; //Se sale por la izq
    //Doble Negro
    else if (lee_lin_der < lin_negra && lee_lin_izq < lin_negra) 
       linea = LIN_NN;
    obstaculo = 0;
    // Movemos ruedas segun la lectura obtenida
    if (obstaculo == 1 ){ //Si encuentro un obstaculo paro
	softPwmWrite(RUEDA_DER, MOV_STOP);
        softPwmWrite(RUEDA_IZQ, MOV_STOP);
    }
    else{// no hay obstaculo	
    	if (linea == LIN_NN){ // Seguimos recto
//	    printf("NN\n");
            softPwmWrite(RUEDA_DER, MOV_HOR);
	    softPwmWrite(RUEDA_IZQ, MOV_AHOR);
        }
        else if (linea == LIN_NB){ // Me estoy saliendo por la derecha
//	    printf("NB\n");
	    softPwmWrite(RUEDA_DER, MOV_STOP);
	    softPwmWrite(RUEDA_IZQ, MOV_AHOR);
	}
        else if (linea == LIN_BN){ // Me estoy saliendo por la izquierda
//	    printf("BN\n");
            softPwmWrite(RUEDA_IZQ, MOV_STOP);
	    softPwmWrite(RUEDA_DER, MOV_HOR); 
        }
	else if(linea == LIN_BB){
//	   printf("BB\n");ç
	   //Giro a la derecha
	   softPwmWrite(RUEDA_DER, MOV_AHOR);
           softPwmWrite(RUEDA_IZQ, MOV_AHOR);
//         int i=0; 
           //Busco la linea negra
           for(i=0;i <200;i++){
//	       printf("BUCLE 1%d\n", i);
               delay(1);   
               if(myAnalogRead(spiChannel,channelConfig,CH_LIN_DER) < lin_negra)
                   break;
           }
	   if(i==200){
	       softPwmWrite(RUEDA_DER, MOV_HOR);
	       softPwmWrite(RUEDA_IZQ, MOV_HOR);
	       for(i = 0; i<400;i++){		
		  // printf("BUCKE 2 %d\n", i);
	           delay(1);
		   if(myAnalogRead(spiChannel,channelConfig,CH_LIN_IZQ) < lin_negra)
		   break;
	       }
	   }
	}
    }
//	   delay(100); 
  /*  if(obstaculo == 1) 
        printf("Obstaculo");
    if(linea == 1 || linea == 2){
        printf("Linea izquierda: %i\n",lee_lin_der);
        printf("Linea derecha: %i\n",lee_lin_izq);
    }
    obstaculo = 0;*/
  }
  return 0;
}
