/******************************************************************************/
/*  Configurando timer 0 e timer 1 para gerar interrupcao a cada timer_tick (us)        */
/******************************************************************************/
/*
** Neste exemplo melhorado, a programacao do timer 0 fica independente da
** programacao do PLL, vc deve apenas informar o valor do cristal utilizado
**
** Parametros de projeto:
** cristal de 10Mhz
** PLL = NAO INTERESSA !!! PODE SER QUALQUER COISA !!!
** VPB clock (clock dos perifericos) = NAO INTERESSA !!! PODE SER QUALQUER COISA !!!
**
** Neste exemplo, suponha que queremos um timer_tick de 250 ms ou 250000 us
**
******************************************************************************/

#include <stdio.h>
#include <string.h> 
#include "type.h"
#include "irq.h"
#include "time.h"
#include "serial.h"
#include "lpc21xx.h"
#include "global.h"
#include "timer.h"
#include "analise.h"
#include "ssp.h"
#include "i2c.h"
//#include "alarme.h"
#include "fsm_master.h"

/*******************************************************************
**
** Frequencia do cristal...
** IMPORTANTE - se voce usar cristal com frequencia diferente,
** ajuste este valor, senao o timer nao funcionara adequadamente
**
********************************************************************/

//===================================================================================
#define APBDIV VPBDIV //so para compatibilizar nomenclatura Keil vs datasheet 
//===================================================================================

#define NUM_TIMERS      1  // Timer 0 e Timer 1

// VARIAVEIS LOCAIS
long delaycount;

void timer_0_ISR(void) __irq 
{
	Master_FSM FSM_state; 
	
	//ontimer = 1;
	static unsigned char TmrIntEcg = 0, FlagInterrupt = 0;
	
	static unsigned char AlarmeFisiologico = 0;
	unsigned char i = 0;
	
	
	if(FlagInterrupt == 0) 
	{
	  
		FlagInterrupt = 1;
		AlarmeFisiologico = gAlarmeFisiologico;
		
	
		//Leitura da chave liga desliga
		if(!(IOPIN0 & BOTAO_ON_OFF))
		{
			gTempoTeclaLiga = 0;
			IO0CLR |= BUZZER;
			
		}
		else if(gTempoTeclaLiga < TEMPO_BOTAO_LIGA)
		{ gTempoTeclaLiga++;
			//IO0SET |= BUZZER;
		}
			
		if(gTempoTeclaLiga >= TEMPO_BOTAO_LIGA)
		{
			//if(gLiberaDesliga)
			//{
				IO0CLR |= RESET_ZIGBEE;//reseta o Zigbee
				//Desliga todos GPIOs
				//IO0DIR  = 0;
				IO0CLR |= ON_OFF;
				IO0SET |= LED_ECG;
				IO0SET |= LED_SPO2;
				IO0SET |= LED_ENF;
				IO0CLR |= BUZZER;
				IO0SET |= LED_ON_BAT;
				gIndexVetor = 0;
				while(1)
				{
					//S� fica preso para o software n�o rodar quando quiser desligar o aparelho.
				}
			//}
			gBotaoDesliga = 1;
		}
		else     
		{
			gBotaoDesliga = 0;
			
			IO0SET |= ON_OFF;
		}
				
		    timerseg++;
				FSM_state.FSM_timer(); // obj maquina
						
		
		FlagInterrupt = 0;
		if(gTempoPiscaLed < 8000) gTempoPiscaLed++;
		else {gTempoPiscaLed = 0;}
			 
	}
					
	T0IR = 1;                             // Clear interrupt flag
	VICVectAddr = 0;                      // Acknowledge Interrupt
	++delaycount;
}

//void timer_1_ISR(void) __irq 
//{
//	
//	switch(gCanalAD)
//	{
//	case	0:
//		ADCR &= 0xFFFFFFF0; // zera sele��o do canal do ad
//		ADCR |= 0x01000001; // inicia leitura no canal 0 do ad
//		gCanalAD = 1;
//		break;
//	case	1:
//		ADCR &= 0xFFFFFFF0; // zera sele��o do canal do ad
//		ADCR |= 0x01000002; // inicia leitura no canal 1 do ad
//		gCanalAD = 0;
//		break;
//	default:
//		break;
//	}
//	
//	T1IR        = 1;                            // Clear interrupt flag
//  VICVectAddr = 0;                            // Acknowledge Interrupt
//}

/** GetCclk
 *
 * @brief Devolve a frequencia de trabalho do chip (CCLK)
 * @brief if (PLL desligado)
 * @brief    CCLK = XTAL_IN_HZ
 * @brief  else
 * @brief    CCLK = XTAL_IN_HZ * PLL_multiplier
 * @return sem retorno (VOID)
 * @see
 */
unsigned long GetCclk(void)
{
  return XTAL_IN_HZ * (PLLCON & 1 ? (PLLCFG & 0xF) + 1 : 1);
}
/** GetVPBdivider
 *
 * @brief Determina a relacao entre clock de processador e clock de perifericos
 * @brief VPBclock = CCLK ou 1/2 CCLK ou 1/4 CCLK ou 
 * @brief VPBclock = CCLK / VPB_divider, onde VPB_divider = enum (1,2,4)
 * @brief VPBDIV eh o registrador 0xE01F_C100 que no datasheet eh chamado de APBDIV
 * @brief mas implementado com o nome VPBDIV pela Keil (vide LPC22xx.h).
 * @return sem retorno (VOID)
 * @see
 */
unsigned long GetVPBdivider(void)
{
  unsigned long VPB_divider = 0;

  switch (APBDIV & 3) {
    case 0: VPB_divider = 4;  break;
    case 1: VPB_divider = 1;  break;
    case 2: VPB_divider = 2;  break;
  }
  return VPB_divider;
}
/** GetPclk
 *
 * @brief Determina o clock dos perifericos (VPBclock)
 * @return sem retorno (VOID)
 * @see
 */
unsigned long GetPclk(void)
{
  return (GetCclk() / GetVPBdivider());
}
/** SetupPLL
 *
 * @brief Programacao do PLL.
 * @brief O PLL aceita frequencias de entrada de 10 a 25Mhz
 * @brief O multiplicador pode ser de 1 a 32, mas na pratica eh limitado pela familia
 * @brief LPC escolhida (no exemplo, LPC2212/01 = 75Mhz maximo)
 *
 * @brief No exemplo, queremos CCLK de 60 Mhz
 *
 * @brief Equacoes: CCLK = XTAL_FREQ * PLL_multiplier
 * @brief           CCLK = Fcco/2/P, onde: 156Mhz <= Fcco <= 320Mhz, e P = enum {1,2,4,8}
 * @brief resolvendo temos: CCLK desejado = 60Mhz, logo PLL_multiplier = 6 ---> (MSELbits = M-1 = 5)
 * @brief            para P = 2 temos Fcco = 240Mhz, logo PSELbits = 01)
 * @brief no registrador PLLCFG teremos entao 0x25
 * @return sem retorno (VOID)
 * @see
 */
#define PLL_lock_bit 0x0400           //lock bit eh o bit.10 de PLLSTAT
void SetupPLL (void) {
  PLLCFG  = 0x25;                     //dos valores calculados previamente...
  PLLCON  = 0x1;                      //habilita PLL
  PLLFEED = 0xAA;
  PLLFEED = 0x55;                     //sequencia de desbloqueio do PLL
// while(PLLSTAT & PLL_lock_bit == 0); //while PLLSTAT.bit.10 == 0, wait....
  PLLCON  = 0x3;                      //conecta saida do PLL ao clock do chip
  PLLFEED = 0xAA;
  PLLFEED = 0x55;                     //sequencia de desbloqueio do PLL
}
/** SetupVPBclock
 *
 * @brief Programacao do clock dos perifericos VPBclock
 * @brief Por default, no power-on-reset, VPB clock = 1/4 CCLK
 *
 * @brief Como programaremos CCLK = 60Mhz, VPB clock = 15Mhz
 *
 * @brief Veja registrador APBDIV (endereco 0xE01F C100)
 * @return sem retorno (VOID)
 * @see
 */
void SetupVPBclock (void) {
  APBDIV = 0x0; //so por desencargo de consciencia
}
/** InitTimer
 *
 * @brief Inicializacao dos timers do sistema
 * @brief O valor de timer_tick em funcao dos parametros do VPBclock e do PLL e�:
 *
 * @brief timer_tick = (T0MR0 * 10e6 * VPB_divider) / XTAL / (MSELbits+1)
 *
 * @brief pior caso possivel (em micro-segundos (us))                                                      
 * @brief timer_tick_maximo = (T0MR0_max * 10e6 * VPB_divider_max) / XTAL_min / (MSELbits_min+1)
 * @brief onde T0MR0_max = 0xFFFFFFFF
 * @brief      VPB_divider_max = 0x4
 * @brief      XTAL_min = 10Mhz
 * @brief      MSELbits_min = 0
 * @brief timer_tick_maximo = 0x66481500 = 1.716.000.000 (us) = 1.716.000 (ms) = 1.716 (s)
 * @param nTimer  0  ->Timer0  1->Timer1
 * @param timer_tick ->Valor em unidades de us
 * @return sem retorno (VOID)
 * @see
 */
int InitTimer (char nTimer,unsigned long timer_tick) {
unsigned long contagens;
unsigned long VPB_divider;        //enum {1, 2 , 4}
unsigned long PLL_multiplier;     //MSEL bits + 1
unsigned long long pre_calc;

  if(timer_tick == 0)
    return -1;  //erro

  //descubro valor de PLL_multiplier
  if(PLLSTAT & 0x200)
    PLL_multiplier = ((PLLSTAT & 0x1f)+1);  //CCLK = XTAL * PLL_multiplier
  else
    PLL_multiplier = 1; //CCLK = XTAL (PLL desligado)

  //pega valor do VPB_divider de clock
  VPB_divider = GetVPBdivider();

  //verifico condi��o de overflow de timer_tick
  //pre_calc = (unsigned long long)(((timer_tick * (XTAL_IN_HZ * PLL_multiplier / VPB_divider)) / 1000000)-1);
  //OBS: SE NAO FIZER O CALCULO NESTA SEQUENCIA, DA MERDA, POIS O EVALUATION EH FEITO EM INT32 QUE PODE DAR
  //OVERFLOW ANTES DO TERMINO DO CALCULO. !!!!!!!
  pre_calc = (unsigned long long) XTAL_IN_HZ;
  pre_calc = pre_calc * (unsigned long long) PLL_multiplier;
  pre_calc = pre_calc / (unsigned long long) VPB_divider;
  pre_calc = pre_calc * (unsigned long long) timer_tick;
  pre_calc = pre_calc / (unsigned long long) 1000000;
  pre_calc = pre_calc - (unsigned long long) 1;

  if(pre_calc > 0xFFFFFFFFull)
    return -1;
  else
    contagens = (unsigned long) pre_calc;

  // Timer 0
  if (nTimer == 0) {
     T0MR0 = (unsigned long) contagens;
     T0MCR = 3;                                  // Interrupt and Reset on MR0
     T0TCR = 1;                                  // Timer0 Enable

     install_irq( TIMER0_INT, (void *)timer_0_ISR);
  	 delaycount=0;
  }
  else {
     T1MR0 = (unsigned long) contagens;
     T1MCR = 3;                                  // Interrupt and Reset on MR1
     T1TCR = 1;                                  // Timer1 Enable
    // install_irq( TIMER1_INT, (void *)timer_1_ISR);

  }
  return 0;

}

/*
 * Delay
 * @brief Aguarda por um tempo determinado.
 * @param nTime Tempo de espera em us
 */

void  Delay(short nDelay) {
	 long nInts;
	 //short x;

	 //x = TIMER_TICK_DESEJADO;
	 //x = (nDelay/TIMER_TICK_DESEJADO);
      // Verifica se a interrupcao esta habilitada
	   
     if ((VICIntEnable&0x00000010)==0)
		 {
				// Inicializa timer
        InitTimer(0,TIMER_TICK_DESEJADO);
				nInts=0;
  	 }
		 else
		 {
				nInts = delaycount;
		 }

		 while (1)
		 {
	     if ((delaycount-nInts)>=(nDelay/TIMER_TICK_DESEJADO))
		     break;
	   }  

}


