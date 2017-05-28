//#define F_CPU 16000000
#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <LiquidCrystal.h>
#include <stdint.h>
#include <PID_v1.h>
#include "PID_v1.cpp"
#include <math.h>

typedef enum param_disp {
	DISP_RUNTIME,
	DISP_PARAM_KP,
	DISP_PARAM_KI,
	DISP_PARAM_KD,
	DISP_TINC,
	DISP_TMEN,
	DISP_TRAC,
	DISP_TSET,
	DISP_RESET,
	DISP_PARAM_COUNT
} param_disp_t;

uint8_t nivelMeniu = 0, duty_cycle = 50;
uint8_t btnOK=13, btnStanga=12, btnCancel=11, btnDreapta=10;
volatile int contor = 0, contor_cit_temp = 0;
volatile int apasat = 0;
volatile int starea_curenta = 0, parametru_modificat = 0;
volatile int v[100], indice_v=-1;
double pwm = 50;
//float kd = 35.1, ki = 23.5, kp = 28.9, tempC = 0;
double kp = 101, ki = 0.2, kd = 0.05, TempC = 0, Tset = 50, Tset_mod = Tset;
param_disp_t dispId = DISP_PARAM_KP;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
int tinit = 0, tfinal = 10, tinc = 30, tmen = 10, trac = 30, tcrt = 0, sec = 0, min = 0, t=0;
double Tinit = 0, Tfinal = 0, deltaT=0, Tmin=0, Tmax=50;


PID myPID(&TempC, &pwm, &Tset_mod, kp, ki, kd, DIRECT);

void printCurrentTime()
{
	sec = (millis()/1000)%60;
	min = millis()/60000;
	tcrt = (millis()/1000)%(tinc + tmen + trac);
	lcd.setCursor(10, 0);
	if(min < 10){
		lcd.print("0");
		lcd.print(min);
	}
	else
		lcd.print(min);
	lcd.print(":");
	if(sec < 10){
		lcd.print("0");
		lcd.print(sec);
	}
	else
		lcd.print(sec);
}

int buton_apasat(uint8_t buton);

unsigned char EEPROM_read(unsigned int uiAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from Data Register */
	return EEDR;
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address and Data Registers */
	EEAR = uiAddress;
	EEDR = ucData;
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMPE);
	/* Start eeprom write by setting EEPE */
	EECR |= (1<<EEPE);
}
// EEPROM_write_genericData(100, &kd, sizeof(kd))
void EEPROM_write_genericData(unsigned int uiAddress, uint8_t *inBuf, uint16_t len)
{
	uint16_t currentIndex = 0;
	while (currentIndex < len)
	{
		EEPROM_write(uiAddress + currentIndex, *(inBuf + currentIndex));
		currentIndex++;
	}
}

void EEPROM_read_genericData(unsigned int uiAddress, uint8_t *outBuf, uint16_t maxlen)
{
	uint16_t currentIndex = 0;
	while (currentIndex < maxlen)
	{
		unsigned char c = EEPROM_read(uiAddress + currentIndex);
		memcpy(outBuf + currentIndex, &c, 1);
		currentIndex++;
	}	
}

void show_runtime()
{	
	lcd.clear();
	//uint8_t seconds = millis()/1000;
	//uint8_t minutes = seconds/60;
	//tcrt += seconds;
	lcd.print("Runtime ");
	printCurrentTime();
	lcd.setCursor(0, 1);
	lcd.print("T:");
	lcd.print(TempC);
	lcd.print("tset:");
	lcd.print(Tset);

	lcd.setCursor(8, 0);
	lcd.print(pwm);
	/*if(tcrt < tinc)
	lcd.print("i");
		else if(tcrt <= (tinc + tmen))
			lcd.print("m");
		else if(tcrt <= (tinc+ tmen + trac))
			lcd.print("r");
	else tcrt = tinit;*/

	//lcd.setCursor(10, 0);
	//lcd.print
	//sa afisam timpul ramas din stadiul la care suntem
}

void resetare()
{
	kp=101;
	ki=0.2;
	kd=0.05;
	tinc=20;
	tmen=10;
	trac=20;
	Tset=50;
}

void meniu_kp()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("1. Kp");
	lcd.setCursor(0, 1);
	lcd.print(kp);
}

void meniu_ki()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("2. Ki");
	lcd.setCursor(0, 1);
	lcd.print(ki);
}

void meniu_kd()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("3. Kd");
	lcd.setCursor(0, 1);
	lcd.print(kd);
}

void citire_butoane_nivel0()
{
	if (buton_apasat(btnDreapta) == 1)
	{
		dispId = (param_disp_t)(dispId + 1);
		dispId = (param_disp_t)(dispId % DISP_PARAM_COUNT);
	}

	else if (buton_apasat(btnStanga) == 1)
	{
		dispId = (param_disp_t)(dispId - 1);
		dispId = (param_disp_t)((DISP_PARAM_COUNT + dispId) % DISP_PARAM_COUNT);
	}

	else if (buton_apasat(btnOK) == 1)
	{
		if(dispId==8)
		{
			nivelMeniu=2;
		}
		else
			nivelMeniu = 1;
	}
}

void meniu_plus()
{
	switch (dispId)
	{
		case DISP_PARAM_KP:
			kp += 0.1;
			break;
		case DISP_PARAM_KI:
			ki += 0.1;
		break;
		case DISP_PARAM_KD:
			kd += 0.1;
		break;
		case DISP_TINC:
		tinc += 1;
		break;
		case DISP_TMEN:
		tmen += 1;
		break;
		case DISP_TRAC:
		trac += 1;
		break;
		case DISP_TSET:
		Tset += 1;
		break;
	}
}

void meniu_minus()
{
	switch (dispId)
	{
		case DISP_PARAM_KP:
			kp -= 0.1;
		break;
		case DISP_PARAM_KI:
			ki -= 0.1;
		break;
		case DISP_PARAM_KD:
			kd -= 0.1;
		break;
		case DISP_TINC:
		tinc -= 1;
		break;
		case DISP_TMEN:
		tmen -= 1;
		break;
		case DISP_TRAC:
		trac -= 1;
		break;
		case DISP_TSET:
		Tset -= 1;
		break;
	}
}

void citire_butoane_nivel1()
{
	if (buton_apasat(btnDreapta) == 1)
	{
		meniu_plus();
	}

	else if (buton_apasat(btnStanga) == 1)
	{
		meniu_minus();
	}

	else if (buton_apasat(btnOK) == 1)
	{
		nivelMeniu = 0;
		//EEPROM_write_genericData(100, (uint8_t*)&kd, sizeof(kd));
	}

	else if (buton_apasat(btnCancel) == 1)
	{
		nivelMeniu = 0;
	}
}

void citire_butoane_nivel2()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Are you sure you want to reset?");
	if(buton_apasat(btnOK)==1)
	{
		resetare();
		nivelMeniu=0;
	}
	else
		if(buton_apasat(btnCancel)==1)
		nivelMeniu=0;

}

void citire_butoane()
{
	switch(nivelMeniu)
	{
		case 0:
		citire_butoane_nivel0();
		break;

		case 1:
		citire_butoane_nivel1();
		break;

		case 2:
		citire_butoane_nivel2();
		break;
	}
}

void meniu_tinc()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("4. Timp inc");
	lcd.setCursor(0, 1);
	lcd.print(tinc);	
}

void meniu_tmen()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("5. Timp men");
	lcd.setCursor(0, 1);
	lcd.print(tmen);
}

void meniu_trac()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("6. Timp rac");
	lcd.setCursor(0, 1);
	lcd.print(trac);
}

void meniu_Tset()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("7. Temp set");
	lcd.setCursor(1, 1);
	lcd.print(Tset);
}

void meniu_reset()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("8. Resetare");
}

void meniu_butoane()
{
	switch(dispId) {

		case DISP_RUNTIME:
			show_runtime();
		break;

		case DISP_PARAM_KP:
			meniu_kp();
		break;

		case DISP_PARAM_KI:
			meniu_ki();
		break;

		case DISP_PARAM_KD:
			meniu_kd();
		break;

		case DISP_TINC:
		meniu_tinc();
		break;

		case DISP_TMEN:
		meniu_tmen();
		break;

		case DISP_TRAC:
		meniu_trac();
		break;

		case DISP_TSET:
		meniu_Tset();
		break;

		case DISP_RESET:
		meniu_reset();
		break;

		default:
		break;
	}
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  /*EEPROM_write(1000, kd);
  EEPROM_write(1002, ki);
  EEPROM_write(1004, kp);*/
  //kd = EEPROM_read(1000);
 // ki = EEPROM_read(1002);
  //kp = EEPROM_read(1004);
  //DDRB = 0x01;
  
  //DDRB = 0x01
  Tinit=(float)analogRead(A0)*500.0/1023.0;
  Tmin=(float)analogRead(A0)*500.0/1023.0;
  Tmax=Tset;
  myPID.SetMode(AUTOMATIC);
  int LED = 9;
  show_runtime();
  pinMode(9, OUTPUT);
  pinMode(btnOK, INPUT);
  pinMode(btnCancel, INPUT);
  pinMode(btnDreapta, INPUT);
  pinMode(btnStanga, INPUT);

  //EEPROM_read_genericData(100, (uint8_t*)&kd, sizeof(kd));
}

int buton_apasat(uint8_t buton)
{
	while((digitalRead(buton)&1) == 0)
	{
		contor++;
		if(contor == 110)
		{
			contor = 0;
			return 1;
		}
	}		
	return 0;
}

void regulator_P(){
	volatile float err = (float)Tset-(float)TempC;
	//pwm = (kp*err) > 255 ? 255 : (kp*err);
	//pwm = (kp*err) < 0 ? 0 : (kp*err);
	if(kp*err>255)
		pwm=255;
		else if(kp*err<0)
				pwm=0;
				else
					pwm=kp*err;
}

double setPoint(double secundaCurenta)
{	if(tcrt%(tinc+tmen+trac)!=0)
		{
			if(tcrt<=tinc)
			{
				//secundaCurenta=0;
				deltaT= abs(Tmax-Tmin);
				tfinal=tinc;
				Tinit=Tmin;
				if(secundaCurenta<=tfinal)
				{
					secundaCurenta++;
					return Tinit+(secundaCurenta/tfinal)*deltaT;
				}
			}
			else if(tcrt<=tinc+tmen)
			{
				tfinal=tmen;
				//secundaCurenta=0;
				if(secundaCurenta<=tfinal)
				{
					secundaCurenta++;
					return Tfinal;
				}
			}
			else if(tcrt<=tinc+tmen+trac)
			{
				//secundaCurenta=0;
				deltaT= abs(Tmax-Tmin);
				tfinal=trac;
				Tinit=Tmax;
				if(secundaCurenta<=tfinal)
				{
					secundaCurenta++;
					return Tinit-(secundaCurenta/tfinal)*deltaT;
				}
			}
		}
		else
		tcrt=0;
}

void aprinde_bec(){
	//int pwm = (duty_cycle/100.00)*255.00;
	//regulator_P();
	if(tcrt == 0 || tcrt == tinc || tcrt == (tinc + tmen) || tcrt == (tinc + tmen + trac))
	{
		t=0;
	}
	if(millis()%1000==0)
	{
		
		t++;
		
	}
	Tset_mod=setPoint(t);
	myPID.Compute();
	analogWrite(9, (int)pwm);
	
	
	//if(tempC >= 29);
		//OCR1A =(duty_cycle/100.00)*255.00;
		//PORTB ^= (1<<9);	
}

void stinge_bec(){
	analogWrite(9, 0);
}

/*void regulator_PI(){
	float sum_err = 0;
	err = (float)tset-(float)tempC;
	sum_err += err;
	pwm = err * kp + sum_err * ki;
}*/

void regulator_PID(){
	
}

float citire_temperatura(){
	int i;
	float temperatura_citita = 0;
	for(i = 0; i < 100; i++)
		temperatura_citita += (float)analogRead(A0)*500.0/1023.0;
	TempC = temperatura_citita/100;
}

void loop() {
	
	meniu_butoane();
	citire_butoane();
	//tempC = analogRead(A0);
	citire_temperatura();
	//if(contor_cit_temp%10 == 0)
	//v[++indice_v]=tempC;
	//tempC = analogRead(A0)/2;
	aprinde_bec();
	//PORTB ^= (1<<9);
	//analogWrite(9, 255);
	//contor_cit_temp++;
	delay(200);	
}

