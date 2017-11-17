#include <Arduino.h>
#include <Cmd.h>
#include <Proto485.h>
#include <SoftwareSerial.h>
#include <Tone.h>


#define TXENABLE 3

#define DEBUG
#ifdef DEBUG
 #define DEBUG_PRINT(x, ...)  Serial.print (x, ##__VA_ARGS__)
 #define DEBUG_PRINTLN(x, ...)  Serial.println (x, ##__VA_ARGS__)
#else
 #define DEBUG_PRINT(x, ...)
 #define DEBUG_PRINTLN(x, ...)  
#endif
bool rpt=false;
bool beepsu485=true;

SoftwareSerial Serial485(2, 4); // RX, TX
Proto485 comm((Stream*)&Serial485,TXENABLE,false);
Tone freq1;

// comandi
void Disarma(int arg_cnt, char **args)
{
    comm.Tx('O',0,0);
}

void Arma(int arg_cnt, char **args)
{
    comm.Tx('P',0,0);
}

void InCasa(int arg_cnt, char **args)
{
    comm.Tx('N',0,0);
}

void startPoll(int arg_cnt, char **args)
{
    rpt=true;
}
void stopPoll(int arg_cnt, char **args)
{
    rpt=false;
}

void MemorizzaParametro(int arg_cnt, char **args)
{
    if(arg_cnt!=3) {Serial.println("numero parametri errati (mp [s|d|i] val)"); return;};
    int v=atoi((const char *)args[2]);
    if(v<1) {Serial.println("parametro valore errato (<0)"); return;};
    byte par[3];
    switch(tolower(args[1][0])) {
        // durata allarme
        case 'd':
            if(v>255) {Serial.println("parametro valore errato (>255)"); return;};
            par[0]='D';
            par[1]=v;
            comm.Tx('Q',2,(const char *)par);
            break;
        // soglia crepuscolare
        case 's':
            if(v>1000) {Serial.println("parametro valore errato (>1000)"); return;};
            par[0]='S';
            par[1]=v & 0xff;
            par[2]=(v >> 8);
            DEBUG_PRINT("vpar=");
            DEBUG_PRINT(par[2],HEX);
            DEBUG_PRINTLN(par[1],HEX);
            comm.Tx('Q',3,(const char *)par);
            break;
        // durata allarme
        case 'i':
            if(v>255) {Serial.println("parametro valore errato (>255)"); return;};
            par[0]='I';
            par[1]=v;
            comm.Tx('Q',2,(const char *)par);
            break;
        // tempo fari
        case 't':
            if(v>255) {Serial.println("parametro valore errato (>255)"); return;};
            par[0]='T';
            par[1]=v;
            comm.Tx('Q',2,(const char *)par);
            break;
    default:
            Serial.println("parametro tipo errato");
            break;
    }
}

void ApriCancello(int arg_cnt, char **args)
{
    comm.Tx('T',0,0);
}

void PingCancello(int arg_cnt, char **args)
{
    comm.Tx('H',0,0);
}

void RichiediStatoTettoia() {
    DEBUG_PRINTLN("rst");
    comm.Tx('L',0,0);
}

void Beep(int arg_cnt, char **args)
{
    if(arg_cnt!=3) {Serial.println("numero parametri errati (be frequenza durata)"); return;};
    freq1.play(atoi((const char *)args[1]), atoi((const char *)args[2]));
}

void AttivaBeep(int arg_cnt, char **args)
{
    beepsu485=true;
}
void DisAttivaBeep(int arg_cnt, char **args)
{
    beepsu485=false;
}
void ElaboraComando(byte comando,byte *par,byte len) {
    DEBUG_PRINT("485 cmd=");
    DEBUG_PRINTLN((char)comando);
    switch(comando) {
        case 'B':
            Serial.println("pirC");
            break;
        case 'Z':
            Serial.println("pirT");
            break;
        case 'T':
            Serial.println("Apricancello");
            break;
        case 'D':
            Serial.println("Notte");
            break;
        case 'E':
            Serial.println("Giorno");
            break;
        case 'G':
            Serial.println("Info da cancello");
            break;
        case 'I':
            Serial.print("Temp=");
            Serial.println((int)(par[0]+par[1]*0x100));
            Serial.print("Hum=");
            Serial.println((par[2]+par[3]*0x100));
            break;
        case 'J':
            Serial.println("Allarme");
            break;
        case 'K':
            Serial.println("Fine Allarme");
            break;
        case 'M':
            Serial.print("Stato tettoia=");
            Serial.print(par[0],HEX);
            Serial.print(":");
            Serial.println(par[1],HEX);
            Serial.print("Durata allarme=");
            Serial.println(par[2],DEC);
            Serial.print("isteresi=");
            Serial.println(par[3],DEC);
            Serial.print("soglia");
            Serial.println(par[4]*0x100+par[5],DEC);
            Serial.print("tempofari=");
            Serial.println(par[6],DEC);
            break;
        case 'R':
            Serial.println("modo non in casa");
            break;
        case 'S':
            Serial.println("modo disarmato");
            break;
        case 'U':
            Serial.println("modo armato");
            break;
    }
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    Serial485.begin(9600);
    cmdInit(&Serial);
    cmdAdd("ar", Arma);
    cmdAdd("di", Disarma);
    cmdAdd("in", InCasa);
    cmdAdd("poll", startPoll);
    cmdAdd("stop", stopPoll);
    cmdAdd("ac", ApriCancello);
    cmdAdd("mp", MemorizzaParametro);
    cmdAdd("be", Beep);
    cmdAdd("ab", AttivaBeep);
    cmdAdd("db", DisAttivaBeep);
    cmdAdd("pc", PingCancello);
    comm.cbElaboraComando=ElaboraComando;
    digitalWrite(TXENABLE, LOW);
    pinMode(TXENABLE, OUTPUT);
    DEBUG_PRINT("F_CPU=");
    DEBUG_PRINTLN(F_CPU,DEC);
    freq1.begin(13);
    freq1.play(1000,1000);
}

void loop() {
    static unsigned long last;
    // put your main code here, to run repeatedly:
    cmdPoll();
    if(Serial485.available()) {comm.ProcessaDatiSeriali(Serial485.read()); if(beepsu485) freq1.play(1000,10);}
    if(rpt) {
        if((millis() -last) > 2000) {RichiediStatoTettoia(); last=millis();}
    }
}

