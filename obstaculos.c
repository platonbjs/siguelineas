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
#define CABECEO 400 // Tiempo que hace cabeceo en el algoritmo de busqueda del camino
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

int main (int argc, char *argv[])
{ 
    int spiChannel=0;
    int channelConfig=CHAN_CONFIG_SINGLE;
    int obstaculo = 0;// 0= sin obstaculo, 1=frontal, 2=izquierda, 3=derecha
    int dis_obstaculo = 200; // Cuanto mas, mas cerca
    int lee_obs_der = 0;
    int lee_obs_izq = 0;
    int cobarde = 2; // 2 = cobarde, 3= no cobarde
    if (argv[1] == 0)
	cobarde = 2;
    else
	cobarde = 3;
    init();

    while(1){
        // Leemos sensores
        lee_obs_der = myAnalogRead(spiChannel,channelConfig,CH_OBS_DER);
        lee_obs_izq = myAnalogRead(spiChannel,channelConfig,CH_OBS_IZQ);
        // Analizamos obstaculos y linea
	//Detectamos obstaculo frontal
        if( lee_obs_der > dis_obstaculo && lee_obs_izq > dis_obstaculo)
            obstaculo = 1; //Detectado obstaculo
        else if(lee_obs_der > dis_obstaculo)
	    obstaculo = 2;
	else if(lee_obs_izq > dis_obstaculo)
	    obstaculo = 3;
	else
	    obstaculo = 0;
	// Movimiento segun lo detectado
	if (obstaculo == 0){
	    softPwmWrite(RUEDA_DER, MOV_HOR);
	    softPwmWrite(RUEDA_IZQ, MOV_AHOR);
	    delay(100);
	}
	// Gira a la derecha
        else if (obstaculo == 1 || obstaculo == cobarde){
	    softPwmWrite(RUEDA_IZQ, MOV_HOR);
	    softPwmWrite(RUEDA_DER, MOV_STOP);
	    delay(100);
	}
	//Gira a la izquierda
	else{
	    softPwmWrite(RUEDA_DER, MOV_AHOR);
	    softPwmWrite(RUEDA_IZQ, MOV_STOP);
	    delay(100);
	}
    }
    return 0;
}
