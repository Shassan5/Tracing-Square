/**
  Generated main.c file from MPLAB Code Configurator
 
  @Company
    Microchip Technology Inc.
 
  @File Name
    main.c
 
  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.
 
  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.171.4
        Device            :  dsPIC33EP128MC502
    The generated drivers are tested against the following:
        Compiler          :  XC16 v2.10
        MPLAB             :  MPLAB X v6.05
*/
 
/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.
 
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
 
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 
    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/
 
/**
  Section: Included Files
*/
#include "mcc_generated_files/system.h"
#define FCY 2000000UL
#include <libpic30.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h> // Needed For Square Root Function sqrt()
 
 
//#define FCY 3685800UL // Operating instruction frequency (define before libpic30)
#define PI 3.141592653589793238462643 // Define PI for calculations
#define PPR 120 // Define PPR for encoder
#define SPR 6400 ///* Define SPR for stepper motor
#define r  0.7935 //0.6815 // Define radius of encoder wheel (4.28" circumference)
#define L 0.31496063 // Define inches traveled per revolution of actuator [inches]
 
 
//Short Variables for keypad pins, R-> Row, C-> Column
#define R1 LATBbits.LATB6
#define R2 LATBbits.LATB7
#define R3 LATBbits.LATB10
#define R4 LATBbits.LATB11
 
#define C1 PORTBbits.RB12
#define C2 PORTBbits.RB13
#define C3 PORTBbits.RB14
#define C4 PORTBbits.RB15
 
//----------------------------Prototypes--------------------------------
void QEIInit (void);
void stepFunc (int32_t stepNum);
void QEIMeas (int32_t *prevMeas, int32_t *currMeas, int32_t *pulseNum);
void updateDist(int32_t *currMeas, double *curPos);
void calcStep (int32_t *stepNum, double *curPos, double *curHeight, double *prevHeight,double W, double H,double R);
char* getKey(); // scan keypad to get the key pressed
//----------------------------------------------------------------------
 
 
bool METRIC=false; //Imperial selected by default
char address=0b01001110; //LCD Address
 
 
//---------------------------------- LCD --------------------------------------
 
//LOWLEVEL CODE FUNCTIONS
void lcd_send_data(char cmd) //sends exact command to LCD using I2C
{
    char data_u,data_l;
    uint8_t data_t[6];
    data_u=(cmd&0xf0);
    data_l = ((cmd<<4)&0xf0);
    data_t[0] = data_u|0x09; //en=1, rs=1
    data_t[1] = data_u|0x0D; // en=0, rs=1
    data_t[2] = data_u|0x09; // en=0, rs=1
    data_t[3] = data_l|0x09; //en=1, rs=1
    data_t[4] = data_l|0x0D; //en=0, rs=1
    data_t[5] = data_l|0x09; //en=0, rs=1
    __delay_ms(2);
    for(int i=0;i<6;i++)
    {
       I2C1CONbits.SEN = 1;        // Start Condition Enable bit -> Initiate Start condition on SDAx and SCLx pins; cleared by module
       while(I2C1CONbits.SEN == 1);// SEN is to be cleared when I2C Start procedure has been completed
       I2C1TRN = address;
       __delay_us(100);
       I2C1TRN = data_t[i];
       __delay_us(100);
       I2C1CONbits.PEN = 1;
       __delay_us(100);
    }
}
void lcd_init_cmd(char cmd) //Low level function used while initializing LCD
{
    char data_u,data_l;
    uint8_t data_t[6];
    data_u=(cmd&0xf0);
    data_l = ((cmd<<4)&0xf0);
    data_t[0] = data_u|0x08; //en=1, rs=0
    data_t[1] = data_u|0x0C; // en=0, rs=0
    data_t[2] = data_u|0x08; // en=0, rs=0
    data_t[3] = data_l|0x08; //en=1, rs=0
    data_t[4] = data_l|0x0C; //en=0, rs=0
    data_t[5] = data_l|0x08; //en=0, rs=0
    for(int i=0;i<6;i++)
    {
       I2C1CONbits.SEN = 1;        // Start Condition Enable bit-> Initiate Start condition on SDAx and SCLx pins; cleared by module
       while(I2C1CONbits.SEN == 1);   // SEN is to be cleared when I2C Start procedure has been completed
       I2C1TRN = address;
       __delay_us(150);
       I2C1TRN = data_t[i];
       __delay_us(100);
       I2C1CONbits.PEN = 1;
       __delay_us(300);
    }
}
void lcd_command(char cmd)//if 28-> 20 24 20 80 84 80, divides command into nibbles to send
{
    char data_u,data_l;
    uint8_t data_t[6];
    data_u=(cmd&0xf0);
    data_l = ((cmd<<4)&0xf0);
    data_t[0] = data_u|0x08; //en=1, rs=0
    data_t[1] = data_u|0x0C; // en=0, rs=0
    data_t[2] = data_u|0x08; // en=0, rs=0
    data_t[3] = data_l|0x08; //en=1, rs=0
    data_t[4] = data_l|0x0C; //en=0, rs=0
    data_t[5] = data_l|0x08; //en=0, rs=0
    for(int i=0;i<6;i++)
    {
       I2C1CONbits.SEN = 1;        // Start Condition Enable bit-> Initiate Start condition on SDAx and SCLx pins; cleared by module
       while(I2C1CONbits.SEN == 1);   // SEN is to be cleared when I2C Start procedure has been completed
       I2C1TRN = address;
       __delay_us(100);
       I2C1TRN = data_t[i];
       __delay_us(100);
       I2C1CONbits.PEN = 1;
       __delay_us(100);
    }
}
 
void sendbits(unsigned int x)//if 37->30 34 30, sends the MSB as 3 nibbles
{
    char data_u,data_l;
    uint8_t data_t[3];
    data_t[0]=x&0xf0;
    data_t[1]=x|0x04;
    data_t[2]=x&0xf0;
    for(int i=0;i<3;i++)
    {
       I2C1CONbits.SEN = 1;        // Start Condition Enable bit -> Initiate Start condition on SDAx and SCLx pins; cleared by module
       while(I2C1CONbits.SEN == 1);   // SEN is to be cleared when I2C Start procedure has been completed
       I2C1TRN = address;
       __delay_us(100);
       I2C1TRN = data_t[i];
       __delay_us(100);
    }
}
void send(char x){ //sends the exact cmd passed to x
    I2C1CONbits.SEN = 1;        // Start Condition Enable bit-> Initiate Start condition on SDAx and SCLx pins; cleared by module
    while(I2C1CONbits.SEN == 1);   // SEN is to be cleared when I2C Start procedure has been completed
    I2C1TRN = address;
    __delay_us(100);
    I2C1TRN = x;
    __delay_us(50);
}//send exact bits
 
 
//HIGH LEVEL CODE FUNCTIONS
void lcd_init() //initialize LCD by sending specific bits
{
    
    __delay_ms(15);
    send(0x00);
    __delay_ms(15);
    sendbits(0x30);
    __delay_ms(5);
    sendbits(0x30);
    __delay_ms(5);
    sendbits(0x30);
    __delay_us(250);
    sendbits(0x20); //4 bit mode
    __delay_us(100);
 
    lcd_init_cmd(0x28); //function set --> DL=0 (4 bit mode), N=1 (2line display) F=0 (5 by 8 pixels)
    //__delay_ms(1);
    lcd_init_cmd(0x0C); //display on/off control --> D=1, C and B = 0 (Cursor and blink, last two bits)
    //__delay_ms(1);
    lcd_init_cmd(0x01); //clear display
    __delay_ms(2);
    lcd_init_cmd(0x06); //entry mode set --> I/D = 1 (increment cursor) & s=0(no shift)
    //__delay_ms(5);
    lcd_init_cmd(0x02); //display on/off --> D=0,C=0,B=0 -->display off
    __delay_ms(2);
    send(0x08);
    //INITIALIZED
}
 
void lcd_print(char c[]) //print string of text
{
    for (int i=0; c[i]!='\0';i++)
    {
        lcd_send_data(c[i]);
    }
}
 
void lcd_printd(double x) //print a double to 3 dp
{
    char str[10];
 
    sprintf(str, "%.3f", x);
    for (int i=0; str[i]!='\0';i++)
    {
        lcd_send_data(str[i]);
    }
}
 
void lcd_printi(int x) //print an int
{
    char str[10];
 
    sprintf(str, "%d", x);
    for (int i=0; str[i]!='\0';i++)
    {
        lcd_send_data(str[i]);
    }
}
 
void lcd_set_cur(int row, int col) // set cursor on the LCD
{
    switch(row)
    {
        case 0: //r1
            col |=0x80;
            break;
        case 1: //r2
            col|=0xC0;
            break;
        case 2: //r3
            col +=0x94;
            break;
        case 3: //r4
            col+=0xD4;
            break;
 
    }
 
    lcd_init_cmd(col);
}
 
void delay(int x){__delay_ms(x);} //Delay function
 
void lcd_clear() { lcd_command(0x01);} //clears LCD
 
//-----------------------------------------------------------------------------
 
 
 
//--------------------------------MENU---------------------------------------------
int selectOption12() //stores selection 1 or 2
{
    delay(500);
    int k=0;
    while(1)
        {
            k=atoi(getKey());
            if(k==1 || k==2)
            {
                return k;
            }
        }
}
 
int selectOption123() //stores selection 1,2 or 3
{
    delay(500);
    int k=0;
    while(1)
        {
            k=atoi(getKey());
            if(k==1 || k==2 || k==3)
            {
                return k;
            }
        }
}
 
void printMenu() //Main menu
{
    lcd_clear();
    lcd_print("Menu:");
    lcd_set_cur(1,0);
    lcd_print("1:Unit");
    lcd_set_cur(2,0);
    lcd_print("2:Trace");
    lcd_set_cur(3,0);
    lcd_print("3:Scribe Adjust");
    delay(500);
}
 
void printStart() //Prints start page
{
    lcd_clear();
    lcd_set_cur(0,3);
    lcd_print("Tracing Square");
    lcd_set_cur(2,2);
    lcd_print("Press 1 to start");
    char *x=getKey();
    while(x!="1")
    {
        x=getKey();
    }
}
  
//----------------------------------------------------------------------------
 
 
 
//----------------------------------KEYPAD------------------------------------
void keypad_init() // set columns high and rows low
{
    //Columns as inputs
    TRISBbits.TRISB15=1;//COLUM4
    TRISBbits.TRISB14=1;//COLUM3
    TRISBbits.TRISB13=1;//COLUM2
    TRISBbits.TRISB12=1;//COLUM1
    //Rows as outputs
    TRISBbits.TRISB11=0;//ROW4
    TRISBbits.TRISB10=0;//ROW3
    TRISBbits.TRISB7=0;//ROW2
    TRISBbits.TRISB6=0;//ROW1
}
 
 
char* getKey()
{
    // sets the rows to read from a specific row (0), then checks for a low. By default they are columns are high
    R1 = 0; //Row1
    R2 = 1;
    R3 = 1;
    R4 = 1;
    if(R1==0 && R2==1 && R3==1 && R4==1)
    {
        if (C1 == 0){ __delay_ms(100); while (C1==0);return "1";}
        if (C2 == 0) { __delay_ms(100); while (C2==0); return "2"; }
        if (C3 == 0) { __delay_ms(100); while (C3==0); return "3"; }
        if (C4== 0) { __delay_ms(100); while (C4==0); return "A"; }
    }
 
    R1 = 1;
    R2 = 0; //Row 2
    R3 = 1;
    R4 = 1;    
    if (C1 == 0){ __delay_ms(100); while (C1==0);return "4";}
    if (C2 == 0) { __delay_ms(100); while (C2==0); return "5"; }
    if (C3 == 0) { __delay_ms(100); while (C3 ==0); return "6"; }
    if (C4== 0) { __delay_ms(100); while (C4==0); return "B"; }
 
    R1 = 1;
    R2 = 1;
    R3 = 0; //Row 3
    R4 = 1;    
    if (C1 == 0){ __delay_ms(100); while (C1==0);return "7";}
    if (C2 == 0) { __delay_ms(100); while (C2==0); return "8"; }
    if (C3 == 0) { __delay_ms(100); while (C3 ==0); return "9"; }
    if (C4== 0) { __delay_ms(100); while (C4==0); return "C"; }
 
    R1 = 1;
    R2 = 1;
    R3 = 1;
    R4 = 0;    //Row 4
    if (C1 == 0){ __delay_ms(100); while (C1==0);return ".";}
    if (C2 == 0) { __delay_ms(100); while (C2==0); return "0"; }
    if (C3 == 0) { __delay_ms(100); while (C3 ==0); return "#"; }
    if (C4 == 0) { __delay_ms(100); while (C4==0); return "."; }
    return "m";
}
 
double getValue2() //returns a double entered on the keypad.
{
    char *x[2];
    char *k=getKey();
    int i=0;
    bool flag=true;
    while(flag)
    {
        k=getKey();
        if(i==2)
        {
            flag=false;
            break;
        }
        else if(k=="#")
        {
            flag=false;
            break;
        }
        if(k!="m")
        {
            x[i]=k;
            i++;
            lcd_print(k);
        }
 
    }
    if(x[0]=="#")
    {
        return 0.0;
    }
 
    int size_of_x = sizeof(x) / sizeof(x[0]);
 
    
    // Concatenate the strings to form "1.25"
    char floatString[10];
    for(int i=0;i<size_of_x;i++)
    {
        floatString[i]=*x[i];
    }
 
 
    // Convert the concatenated string to float using atof
    double result = atof(floatString);
 
    return result;
}
 
double getValue() //returns a double entered on keypad
{
    char *x[10];
    char *k=getKey();
    int i=0;
    bool flag=true;
    while(flag)
    {
        k=getKey();
        if(k=="#")
        {
            flag=false;
            break;
        }
        if(k!="m")
        {
            x[i]=k;
            i++;
            lcd_print(k);
        }  
    }
    if(x[0]=="#")
    {
        return 0.0;
    }
 
    // Concatenate the strings to form "1.25"
    char floatString[10];
    for(int k=0;k<i;k++)
    {
        floatString[k]=*x[k];
    }
 
    double result = atof(floatString);
 
    return result;
}
 
void getValueHash() //check if "#" is pressed
{
    bool flag=true;
    char *k=getKey();
    while(flag)
    {
        k=getKey();
        if(k=="#")
        {
            flag=false;
        }
    }
}
 
void trace(double W, double H) //trace function, also displays progress
{
    LATBbits.LATB4 = 0; // Set Pin 11 High (Initialized High - Driver enabled)
    double R = (H/2) + ((W*W)/(8*H)); // Radius of circle the arch follows
    //== VARIABLES DECLARATION FOR MOTOR CONTROL ALGORITHM =====================================
    int32_t stepNum = 0; // Value Of stepNum Determines How Many Steps The Motor Is Instructed To Actuate While In stepFunc()
    int32_t prevMeas = 0; // Value Of Position Register On Previous Sample
    int32_t currMeas = 0; // Value Of Position Register On Current Sample
    int32_t pulseNum = 0; // Number Of Encoder Pulses Since Previous Sample
    double curPos = 0;  // Current Horizontal Position Of Machine Relative To Start Position
    double curHeight = 0; // Current Height Of Scribe
    double prevHeight = 0; // Previous Height Of Scribe
    //=== END OF VARIABLES DECLARATION FOR MOTOR CONTROL ALGORITHM ================================
    QEIInit (); // Initialize QEI Module
    int i=0;
    int iprev=0;
    lcd_clear();
    lcd_print("Tracing...");
    lcd_set_cur(1,0);
    lcd_print("progress:");
    lcd_set_cur(1,11);
    lcd_print("%");
    while(curPos<W)
    {
        i=(curPos/W)*100;
        if(i!=iprev)
        {
            iprev=i;
            if(iprev<10)
            {
                lcd_set_cur(1,10);
                lcd_print(" ");
            }
            lcd_set_cur(1,9);
            lcd_printi(i);
        }
        
        QEIMeas(&prevMeas, &currMeas, &pulseNum);
        updateDist(&currMeas, &curPos);
        calcStep (&stepNum, &curPos, &curHeight, &prevHeight,W, H,R);
        stepFunc(stepNum);
        Nop();
    }
    lcd_set_cur(1,9);
    lcd_print("100%");
    delay(200);
    for(int k=0;k<3;k++)
    {
        lcd_clear();
        delay(200);
        lcd_print("Trace Complete!");
        delay(500);
    }
    
    LATBbits.LATB4 = 1; // Set Pin 11 High (Initialized High - Driver Disabled)
 
}
 
 
void lcd_blink_on(){lcd_init_cmd(0x0F);}
void lcd_blink_off(){lcd_init_cmd(0x0C);}
 
//-----------------------------------------------------------------------------
 
int main(void){
 
 
    // initialize the device
    TRISBbits.TRISB8=0;
    TRISBbits.TRISB9=0;
    ANSELBbits.ANSB8=0;
    ANSELB=0;
 
    QEIInit (); // Initialize QEI Module
    SYSTEM_Initialize();
    lcd_init();
    keypad_init();
    //=== Stepper Motor Driver Pin Initialization ==============================================
    ANSELAbits.ANSA0 = 0; // Disable analog on Pin 2
    TRISAbits.TRISA0 = 0; // Configure Pin 2 as Output (Step Control Signal)
    LATAbits.LATA0 = 0; // Set Pin 2 Low (Initialized Low)
    ANSELAbits.ANSA1 = 0; // Disable analog on Pin 3
    TRISAbits.TRISA1 = 0; // Configure Pin 3 as Output (Direction Control Signal)
    LATAbits.LATA1 = 0; // Set Pin 3 Low (Initialized Low)
    TRISBbits.TRISB4 = 0; // Configure Pin 11 as Output (Stepper Driver Enable Control Signal)
    LATBbits.LATB4 = 1; // Set Pin 11 High (Initialized High - Driver Disabled)
    //=== End Of Stepper Motor Driver Pin Initialization ========================================
    
    lcd_clear();
    lcd_command(0x02);
    lcd_set_cur(0,5);
    lcd_print("Welcome!");
    delay(3000);
    lcd_clear();
 
    while (1)
    {  
        //start:
        printStart(); //prints Start Menu
        menu:
            printMenu(); //prints MainMenu
            int k=selectOption123(); //option select
 
        switch(k) //based on selection, go to case k
        {
            case 1: //Unit Select
                lcd_clear();
                lcd_set_cur(0,0);
                lcd_print("1:Metric");
                lcd_set_cur(1,0);
                lcd_print("2:Imperial");
 
                int k=selectOption12();
                if(k==1) //if 1 is pressed select Metric, 2 for Imperial
                {
                    METRIC=true;
                    lcd_clear();
                    lcd_print("Metric Selected!");
                    delay(1500);
                    lcd_clear();
                    goto menu;
                }
                else if(k==2)
                {
                    METRIC=false;
                    lcd_clear();
                    lcd_print("Imperial Selected!");
                    delay(1500);
                    lcd_clear();
                    goto menu;
                }  
            case 2: //Trace Menu
                lcd_clear();
                lcd_set_cur(0,0);
                lcd_print("1:Arc");
                lcd_set_cur(1,0);
                lcd_print("2:Tri");
                lcd_set_cur(2,0);
                lcd_print("3:Stringers");
                k=selectOption123();
 
                double heightInches;
                double widthInches;
                if (k==1) //Trace Arc
                {
                    //double imperialHfeet=0;
                    double imperialHinch=0;
                    double imperialHinchfrac1=0;
                    double imperialHinchfrac2=0;
                    double imperialWfeet=0;
                    double imperialWinch=0;
                    double imperialWinchfrac1=0;
                    double imperialWinchfrac2=0;
                    if(!METRIC) //if unit is Imperial
                    {
                        imperial:
                            lcd_clear();
                            lcd_print("Unit: Imperial/in"); //Pre-trace input menu
                            lcd_set_cur(1,0);
                            lcd_print("Height:");
                            lcd_set_cur(2,0);
                            lcd_print(" Inches:  -  /  ");
                            lcd_set_cur(2,8);
                            lcd_blink_on();
 
                            imperialHinch=getValue2(); //Get Height in Inches
                            lcd_set_cur(2,11);
                            imperialHinchfrac1=getValue2(); //Get Height fraction1
                            lcd_set_cur(2,14);
                            imperialHinchfrac2=getValue2(); //Get Height fraction2
 
 
                            //------------------------------
 
                            lcd_clear();
                            lcd_print("Unit: Imperial/ft-in"); //Similar commands to height but for width
                            lcd_set_cur(1,0);
                            lcd_print("Width:");
                            lcd_set_cur(2,0);
                            lcd_print(" Feet:");
                            lcd_set_cur(3,0);
                            lcd_print(" Inches:  -  /  ");
                            lcd_set_cur(2,6);
                            lcd_blink_on();
 
                            imperialWfeet=getValue2(); //get width in feet             
                            lcd_set_cur(3,8);
                            imperialWinch=getValue2(); //get width inches
                            lcd_set_cur(3,11);
                            imperialWinchfrac1=getValue2(); //get width frac1
                            lcd_set_cur(3,14);
                            imperialWinchfrac2=getValue2(); //get width frac2
                            lcd_blink_off();
 
                            //-----------------------------
 
                            if(imperialHinchfrac2==0) //division by 0 check
                            {
                                heightInches=imperialHinch;
                            }
                            else //divide otherwise
                            {
                                heightInches=imperialHinch+(imperialHinchfrac1/imperialHinchfrac2);
                            }
 
                            if(imperialWinchfrac2==0) //division by 0 check and width in inches computation
                            {
                                widthInches=(imperialWfeet*12)+imperialWinch;
                            }
                            else
                            {
                                widthInches=(imperialWfeet*12)+imperialWinch+(imperialWinchfrac1/imperialWinchfrac2);
                            }
                            if(heightInches==0 || widthInches==0 || heightInches>10 || widthInches>240 || widthInches<2*heightInches)
                            {
                                lcd_clear();
                                lcd_print("HeightMax:10in");
                                lcd_set_cur(1,0);
                                lcd_print("WidthMax:20ft");
                                lcd_set_cur(2,0);
                                lcd_print("Width<2*Height");
                                lcd_set_cur(3,0);
                                lcd_print("Re-Enter Dimensions!");
                                getValueHash();
                                goto imperial;
                            }
                    }
                    else //if unit is metric, same comments as above apply.
                    {
                        metric:
                            lcd_clear();
                            double metricH=0;
                            double metricW=0;
                            lcd_print("Unit: Metric/m");
                            lcd_set_cur(1,0);
                            lcd_print("Height: ");
                            lcd_set_cur(2,0);     
                            lcd_print("Width: ");
                            lcd_set_cur(1,8);
                            lcd_blink_on();
                            metricH=getValue();
                            lcd_set_cur(2,8);
                            metricW=getValue();
                            lcd_blink_off();
                            delay(1000);   
 
                            heightInches=metricH*39.3701; //change to metric
                            widthInches=metricW*39.3701;
 
                            if(heightInches==0 || widthInches==0 || heightInches>10 || widthInches>240 || widthInches<2*heightInches)//Invalid height width
                                {
                                    lcd_clear();
                                    lcd_print("HeightMax:0.254m");
                                    lcd_set_cur(1,0);
                                    lcd_print("WidthMax:6.096m");
                                    lcd_set_cur(2,0);
                                    lcd_print("Width<2*Height");
                                    lcd_set_cur(3,0);
                                    lcd_print("Re-Enter Dimensions!");
                                    getValueHash();
                                    goto metric;
                                }
                            lcd_clear();
                    }
                    lcd_clear();
                    lcd_print("Proceed to trace?");
                    lcd_set_cur(1,0);
                    lcd_print("1:Yes");
                    lcd_set_cur(2,0);
                    lcd_print("2:No");
                    int h=selectOption12();
                    if(h==2)
                    {
                        goto menu;
                    }
                    trace(widthInches,heightInches); //Trace
                }
                if(k==2)
                {
                }
                if(k==3)
                {
                }
                goto menu;
                break;
                
            case 3: //Scribe adjust menu
                lcd_clear();
                lcd_set_cur(0,0);
                lcd_print("1:Up");
                lcd_set_cur(1,0);
                lcd_print("2:Down");
                lcd_set_cur(2,0);
                lcd_print("3:Exit");
                
                bool flag=true;
                
                R1 = 0; //Set-up to read row 1
                R2 = 1;
                R3 = 1;
                R4 = 1;
                LATBbits.LATB4 = 0; // Set Pin 11 High (Initialized High - Driver Enabled)
                while(flag)
                {
                   if(R1==0 && R2==1 && R3==1 && R4==1)
                    {
                        if (C1 == 0) //if 1 is pressed move up
                        {
                            LATAbits.LATA1 = 1;
                            
                            LATAbits.LATA0 = 1; // Set Pin 2 High (Step Control Pin)
                            __delay_us(50);
                            LATAbits.LATA0 = 0; // Set Pin 2 Low (Step Control Pin)
                            __delay_us(50);
                            
                            
                        }
                        if (C2 == 0)  //if 2 is pressed move down
                        {
                            LATAbits.LATA1 = 0;
                            
                            LATAbits.LATA0 = 1; // Set Pin 2 High (Step Control Pin)
                            __delay_us(50);
                            LATAbits.LATA0 = 0; // Set Pin 2 Low (Step Control Pin)
                            __delay_us(50);
                               
                        }
                        if(C3 == 0)
                        {
                            LATBbits.LATB4 = 1; // Set Pin 11 High (Initialized High - Driver Disabled)
                            goto menu;
                            break;
                        }
                    }
                }
            default:
                k=atoi(getKey());
        }      
        lcd_clear();
    }
    return 1;
}
 
 
 
void QEIInit (void)
{
    ANSELBbits.ANSB0 = 0; // Disables Analog Driver (PIN4)
    ANSELBbits.ANSB1 = 0; // Disables Analog Driver (PIN5)
    QEI1CONbits.QEIEN = 1; // Enables QEI Module
    RPINR14bits.QEA1R = 0b0100000; // RB0 (PIN4)       
    RPINR14bits.QEB1R = 0b0100001; // RB1 (PIN5)       
    POS1HLD = 0x0000; // Clears Position (High) Register (Position Hold Must Be Loaded Before Position Low)
    POS1CNTL = 0x0000; // Clears Position (Low) Register (When Low Is Loaded, The Hold Register Loads Position High Simultaneously)
}
void stepFunc (int32_t stepNum)
{
   // Should add a condition that when stepNum == 0, the driver is not enabled to save power.
    if (stepNum == 0)
    {
        return;
    }
    //LATBbits.LATB4 = 0;  // Enable Stepper Driver
    if(stepNum < 0)
    {
        LATAbits.LATA1 = 0; // Rotate Clockwise (Decrease Height Of Scribe)
        stepNum = stepNum*(-1); // For Loop Below Needs Positive Number For Condition
    }
    else
    {
        LATAbits.LATA1 = 1; // Rotate Counter Clockwise (Increase Height Of Scribe)
    }
    // The Below For Loop Bit-Bangs Motor Control Signal
    for (uint32_t i = 0; i < stepNum; i++)
    {
      LATAbits.LATA0 = 1; // Set Pin 2 High (Step Control Pin)
      __delay_us(22);
      LATAbits.LATA0 = 0; // Set Pin 2 Low (Step Control Pin)
      __delay_us(22);
    }
     //LATBbits.LATB4 = 1;  // Disable Stepper Driver
return;
}
void QEIMeas (int32_t *prevMeas, int32_t *currMeas, int32_t *pulseNum)
{
    int test = 0; // Used for debugging only
    uint32_t posLow = POS1CNTL; // Position Low Register Read
    uint32_t posHigh = POS1CNTH; // Position High Register Read
    *prevMeas = *currMeas; // Update Previous Position Register Measurement (Combined Position High & Low)
    *currMeas = ((posHigh << 16) | posLow); // Current Position Register Measurement (Combined Position High & Low)
    // The Below If Statement Doesn't Allow The Current Measurement To Read Negative Values  
   if(*currMeas < 0)
    {
        *currMeas = 0;
    }
    // Start Of Debugging Code: pulseNum Is Used For Debugging Purposes Only Allowing For The Determination Of Proper Encoder Functions/Readings
    *pulseNum = (*currMeas - *prevMeas)/4; // Used for debugging only
    if (*pulseNum < 0) // Used for debugging only
    {
        test = (*pulseNum * (-1));
    }
    // End Of Debugging Code =========================================================================================================
    return;
}
void updateDist(int32_t *currMeas, double *curPos)
{
    *curPos = (PI*r*(*currMeas))/(2*PPR); // Updates Current Horizontal Position Relative To The Starting Position
    return;
}
void calcStep (int32_t *stepNum, double *curPos, double *curHeight, double *prevHeight,double W, double H,double R)
{
    double desirHeight; // Desired Height Based On Current Horizontal Position
    double Pos; // Dummy Position Variable For Below If/Else Statement
    // The Below If/Else Statements Doesn't Allow The Current Position Measurement To Exceed The Desired Width Which Would Cause The Motor To Actuate Beyond The Desired Position
    if (*curPos > W)
    {
        Pos = W;
    }
    else
    {
        Pos = *curPos;
    }
    double A = Pos - (W/2); // Must come up with a better name
    *prevHeight = *curHeight; // Update Previous Height
    desirHeight =  sqrtf((R*R) - (A*A)) + H - R; // Desired Height Based On Current Horizontal Position
    *stepNum = round((desirHeight - *prevHeight)*SPR/L); // Required Number Of Steps To Get To Desired Height
    *curHeight = *curHeight + L*(*stepNum)/SPR; // Current Height Based On stepNum
    return;
}
/**
End of File
*/