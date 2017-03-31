//#define F_CPU 16000000
#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <LiquidCrystal.h>
//#include <EEPROM.h>

/*#define btnOK PORTB4*/
uint8_t btnOK=12, btnCancel=10, btnDreapta=9, btnStanga=11;
volatile int contor = 0;
volatile int apasat = 0;
volatile int starea_curenta = 0, parametru_modificat = 0;
float kd = 35.1, ki= 23.0, kp = 28.9;


LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
int tinit = 0, tinc = 15, tmen = 20, trac = 12, tcrt = tinit;

unsigned char EEPROM_read(unsigned int uiAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE))
	;
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

void show_runtime()
{	
	uint8_t seconds = millis()/1000;
	uint8_t minutes = seconds/60;
	tcrt += seconds;
	lcd.print("Runtime ");
	lcd.setCursor(0, 1);
	lcd.print("T:20.7 ");
	lcd.print("tset:25.5");

	lcd.setCursor(8, 0);
	if(tcrt < tinc)
	lcd.print("i");
		else if(tcrt <= tinit + tmen)
			lcd.print("m");
		else if(tcrt <= tinit + tmen + trac)
			lcd.print("r");
	else tcrt = tinit;
	//lcd.setCursor(10, 0);
	//lcd.print
	//sa afisam timpul ramas din stadiul la care suntem
}

void meniu_butoane()
{
	//int val = digitalRead(PORTB5);
	//if(val == 1)
	//{

	lcd.setCursor(1, 0);
	lcd.print("Kd");
	lcd.setCursor(7, 0);
	lcd.print("Ki");
	lcd.setCursor(13, 0);
	lcd.print("Kp");
	lcd.setCursor(0, 1);
	lcd.print(kd);
	lcd.setCursor(6, 1);
	lcd.print(ki);
	lcd.setCursor(12, 1);
	lcd.print(kp);
	

	//}
}


void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  kd = EEPROM_read(1000);
  ki = EEPROM_read(1002);
  kp = EEPROM_read(1004);
  show_runtime();
  pinMode(btnOK, INPUT);
  pinMode(btnCancel, INPUT);
  pinMode(btnDreapta, INPUT);
  pinMode(btnStanga, INPUT);

  //btnOK = 1;

  /**/

	
 }

void pune_cursor(float parametru)
{
	if(parametru == kd)
		lcd.setCursor(0, 0);
	else if (parametru == ki)
		lcd.setCursor(6, 0);
	else if(parametru == kp)
		lcd.setCursor(12, 0);
}

void schimbare_parametru(float parametru)
{
	starea_curenta = 2;

		//while(starea_curenta == 2)
		//{
			if(buton_apasat(btnDreapta) == 1 && starea_curenta == 2)
			{
				parametru += 0.1;
				if(parametru == kd)
					parametru_modificat = 1;
					else if(parametru == ki)
						parametru_modificat = 2;
						else if(parametru == kp)
							parametru_modificat = 3;
				pune_cursor(parametru);
				lcd.print(parametru);
			}
			else if(buton_apasat(btnStanga) == 1 && starea_curenta == 2)
			{
				parametru -= 0.1;
				if(parametru == kd)
					parametru_modificat = 1;
					else if(parametru == ki)
						parametru_modificat = 2;
						else if(parametru == kp)
							parametru_modificat = 3;
				pune_cursor(parametru);
				lcd.print(parametru);
			}
			else if (buton_apasat(btnOK) && starea_curenta == 2)
				{
					switch(parametru_modificat)
						case 0: break;
						case 1: EEPROM_write(1000, kd); break;
						case 2: EEPROM_write(1002, ki); break;
						case 3: EEPROM_write(1004, kp); break;
				}
			else if(buton_apasat(btnCancel) == 1 && starea_curenta == 2)
			{
				starea_curenta = 1;
			}
		}
}

int buton_apasat(uint8_t buton)
{
	while((digitalRead(buton)&1)== 0)
	{
		contor++;
		if(contor == 127)
		{
			return 1;
			contor = 0;
		}
	}
		
	return 0;
}
void loop() {
//show_runtime();
	if(buton_apasat(btnOK) == 1)
	{
		lcd.clear();		
		meniu_butoane();
		starea_curenta = 1;
	}
	
	if(starea_curenta == 1)
	{
		if(buton_apasat(btnOK) == 1)
		{
			pune_cursor(kd);
			schimbare_parametru(kd);
		}
		if(buton_apasat(btnDreapta) == 1)
		{
				pune_cursor(ki);
				if(buton_apasat(btnOK) == 1)
					schimbare_parametru(ki);
				else if(buton_apasat(btnDreapta) == 1)
				{
					pune_cursor(kp);
					if(buton_apasat(btnOK) == 1)
						schimbare_parametru(kp);
					else if(buton_apasat(btnStanga) == 1)
					{
						pune_cursor(ki);
						if(buton_apasat(btnOK) == 1)
							schimbare_parametru(ki);
						else if(buton_apasat(btnStanga) == 1)
						{
							pune_cursor(kd);
							if(buton_apasat(btnOK) == 1)
								schimbare_parametru(kd);
						}							
					}
				}
			}
			
		if(buton_apasat(btnCancel) == 1)
		{
			lcd.clear();
			show_runtime();
			starea_curenta = 0;
		}	
	}
	
}

