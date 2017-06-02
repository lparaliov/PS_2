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
volatile int contor = 0;
volatile int v[100], indice_v=-1;
double pwm = 50;
double kp = 101, ki = 2, kd = 0, TempC = 0, Tset = 50, Tset_mod = Tset, kp_mom = kp, ki_mom = ki, kd_mom = kd;

param_disp_t dispId = DISP_RUNTIME;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
int tinit = 0, tfinal, tinc = 60, tmen = 10, trac = 60, tcrt = 0, sec = 0, min = 0, t = 0,  tinc_mom = tinc, tmen_mom = tmen, trac_mom = trac, Tset_mom = Tset;
double Tinit = 0, Tfinal = 0, deltaT = 0, Tmin = 0, Tmax = Tset;


PID myPID(&TempC, &pwm, &Tset_mod, kp, ki, kd, DIRECT);

void printCurrentTime()
{
	sec = (millis()/1000)%60;
	min = millis()/60000;
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
	lcd.print("Runtime ");
	printCurrentTime();
	lcd.setCursor(0, 1);
	lcd.print("T:");
	lcd.print(TempC);
	lcd.print("tset:");
	lcd.print(Tset);

	lcd.setCursor(8, 0);
	//lcd.print(pwm);
	if(tcrt < tinc)
		lcd.print("i");
		else if(tcrt <= (tinc + tmen))
			lcd.print("m");
		else if(tcrt <= (tinc+ tmen + trac))
			lcd.print("r");
}


void stinge_bec(){
	analogWrite(9, 0);
}

void resetare()
{
	kp=101;
	ki=2;
	kd=0;
	tcrt = 0;
	tinc=60;
	tmen=10;
	trac=60;
	Tset=50;
	stinge_bec();
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
		dispId = (param_disp_t)((DISP_PARAM_COUNT + dispId - 1) % DISP_PARAM_COUNT);
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

void cancel_button(){
	switch(dispId){
		case DISP_PARAM_KP:
			kp = kp_mom;
			break;
		
		case DISP_PARAM_KI:
			ki = ki_mom;
			break;

		case DISP_PARAM_KD:
			kd = kd_mom;
			break;

		case DISP_TINC:
			tinc = tinc_mom;
			break;

		case DISP_TMEN:
			tmen = tmen_mom;
			break;

		case DISP_TRAC:
			trac = trac_mom;
			break;

		case DISP_TSET:
			Tset = Tset_mom;
			break;
	}
}

void scrie_EEPROM(){
	switch (dispId)
	{
		case DISP_PARAM_KP:
		EEPROM_write_genericData(100, (uint8_t*)&kp, sizeof(kp));
		break;
		
		case DISP_PARAM_KI:
		EEPROM_write_genericData(200, (uint8_t*)&ki, sizeof(ki));
		break;

		case DISP_PARAM_KD:
		EEPROM_write_genericData(300, (uint8_t*)&kd, sizeof(kd));
		break;

		case DISP_TINC:
		EEPROM_write_genericData(400, (uint8_t*)&tinc, sizeof(tinc));
		break;

		case DISP_TMEN:
		EEPROM_write_genericData(500, (uint8_t*)&tmen, sizeof(tmen));
		break;

		case DISP_TRAC:
		EEPROM_write_genericData(600, (uint8_t*)&trac, sizeof(trac));
		break;

		case DISP_TSET:
		EEPROM_write_genericData(700, (uint8_t*)&Tset, sizeof(Tset));
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
		lcd.setCursor(5, 0);
		lcd.print("Save to EEPROM?");
		if(buton_apasat(btnOK) == 1 ){
			scrie_EEPROM();
		}
	}

	else if (buton_apasat(btnCancel) == 1)
	{
		nivelMeniu = 0;
		cancel_button();
	}
}

void citire_butoane_nivel2()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Are you sure you");
	lcd.print("want to reset?");
	if(buton_apasat(btnOK) == 1)
	{
		resetare();
		nivelMeniu = 0;
	}
	else
		if(buton_apasat(btnCancel) == 1)
		nivelMeniu = 0;

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

		default: show_runtime();
		break;
	}
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  /*EEPROM_write(1000, kd);
  EEPROM_write(1002, ki);
  EEPROM_write(1004, kp);*/
	EEPROM_read_genericData(300, (uint8_t*)&kd, sizeof(kd));
	EEPROM_read_genericData(200, (uint8_t*)&ki, sizeof(ki));
    EEPROM_read_genericData(100, (uint8_t*)&kp, sizeof(kp));
	EEPROM_read_genericData(400, (uint8_t*)&tinc, sizeof(tinc));
	EEPROM_read_genericData(500, (uint8_t*)&tmen, sizeof(tmen));
	EEPROM_read_genericData(600, (uint8_t*)&trac, sizeof(trac));
	EEPROM_read_genericData(700, (uint8_t*)&Tset, sizeof(Tset));
 
  Tinit = (float)analogRead(A0)*500.0/1023.0;
  //Tmin = (float)analogRead(A0)*500.0/1023.0;
  Tmin = Tinit;
  Tmax = Tset;
  myPID.SetMode(AUTOMATIC);
  int LED = 9;
  show_runtime();
  pinMode(9, OUTPUT);
  pinMode(btnOK, INPUT);
  pinMode(btnCancel, INPUT);
  pinMode(btnDreapta, INPUT);
  pinMode(btnStanga, INPUT);
  Serial.begin(9600);
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
{	//if(tcrt%(tinc+tmen+trac)!=0)
		//{
			tcrt = (millis()/1000)%(tinc + tmen + trac);
			if(tcrt <= tinc)
			{
			Serial.print(tcrt);
			Serial.print("  ");
			Serial.print(tinc);
			Serial.println("inc");
				//secundaCurenta=0;
				deltaT= abs(Tmax-Tmin);
				tfinal = tinc;
				Tinit = Tmin;
				Tfinal = Tmax;
				Serial.print(tfinal);
				Serial.print("  ");
			    Serial.print(deltaT);		
				Serial.print("  ");
				Serial.print(secundaCurenta);		
				Serial.print("  ");
				Serial.print(Tinit);		
				Serial.println("  ");
				if(secundaCurenta <= tfinal)
				{
					//secundaCurenta++;
					return Tinit+( secundaCurenta/tfinal)*deltaT;
				}
			}
			else if(tcrt > tinc && tcrt <= tinc+tmen)
			{
				Serial.println("men");
				tfinal = tmen;
				//secundaCurenta=0;
				if(secundaCurenta <= tfinal)
				{
					//secundaCurenta++;
					return Tfinal;
				}
			}
			else if(tcrt > tinc + tmen && tcrt <= tinc + tmen + trac)
			{
				Serial.println("rac");
				//secundaCurenta=0;
				deltaT= abs(Tmax-Tmin);
				tfinal = trac;
				Tinit = Tmax;
				Tfinal = Tmin;
				if(secundaCurenta <= tfinal)
				{
					//secundaCurenta++;
					return Tinit-(secundaCurenta/tfinal)*deltaT;
				}
			}
		//}
		//else
		// tcrt = tcrt%(tinc+tmen+trac);
}

void aprinde_bec(){
	static uint32_t lastTime = 0;
	//int pwm = (duty_cycle/100.00)*255.00;
	//regulator_P();
	if(tcrt == 0 || tcrt == tinc || tcrt == (tinc + tmen) || tcrt == (tinc + tmen + trac))
	{
		t = 0;
	}
	if(millis() > (lastTime + 1000))
	{		
		t++;
		lastTime=millis();
	}
	Tset_mod = setPoint(t);
	//Serial.println(Tset_mod);
	myPID.Compute();
	analogWrite(9, (int)pwm);
}


/*void regulator_PI(){
	float sum_err = 0;
	err = (float)tset-(float)tempC;
	sum_err += err;
	pwm = err * kp + sum_err * ki;
}*/


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
	citire_temperatura();
	aprinde_bec();
	delay(200);	
}

