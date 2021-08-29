#include <LiquidCrystal.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ALib0.h>
#include <SD.h>
#include <Wire.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <HttpClient.h>
#include <string.h>
#include <EthernetUdp.h>




//########################################
// Deklaration Globale Variablen
//########################################

EthernetClient client;

int    HTTP_PORT   = 5000;
String HTTP_METHOD = "GET"; // or POST
char   HOST_NAME[] = "192.168.178.153";
String PATH_NAME   = "/stop";

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 4, 2);
IPAddress RecipientIP(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress sServerAddr(192, 168, 4, 1);

unsigned int localPort = 9000;
unsigned int RecipientPort = 5100;

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;



char sUser[] = "messmodul";              // MySQL user login username
char sPassword[] = "Jockel01.";        // MySQL user login password

MySQL_Connection conn((Client *)&client);

const byte interruptPin = 2;
const float fDistPerImp = ((150.0 * 3.1415) / 23.0);
volatile float liStation;
volatile float ilStationWrite;
volatile float iHoehe;
volatile int iHoeheWrite;
volatile int iMaxHoehe;
volatile float lVelWrite;
volatile bool bSave00;
volatile bool bSave01;
volatile bool bMessungAktiv;
volatile bool bSendView;
bool bKaliAktiv;
bool bSdRead;
static int iIdle;

volatile int aHohe[100][2];
volatile int aVel[100][2];
volatile float aStation[100][2];
volatile long lTimeUsedMax;
static unsigned long ulLastTime;
static unsigned long uiMillis;

volatile static int iIndex;
volatile int iPointer;
long lMillis;
long lTimeUsed;
  
File dataFile;
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):

EthernetServer server(80);


const int chipSelect = 4;
const int bLedImp = 7;
const int bLedRun = 6;


//#################################################
// Setup
//#################################################
void setup() {


  //#################################################
  //Interrupt
  //#################################################
  attachInterrupt(digitalPinToInterrupt(interruptPin), fMeasure, RISING);

  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  Ethernet.setLocalIP(ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }



  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(bLedImp, OUTPUT);
  pinMode(bLedRun, OUTPUT);

  if (!SD.begin(chipSelect)) {

    Serial.println("initialization failed. Things to check:");

    Serial.println("1. is a card inserted?");

    Serial.println("2. is your wiring correct?");

    Serial.println("3. did you change the chipSelect pin to match your shield or module?");

    Serial.println("Note: press reset or reopen this serial monitor after fixing your issue!");

    while (true);

  }

  if (conn.connect(sServerAddr, 3306, sUser, sPassword)) {
    delay(1000);
    // Initiate the query class instance
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  }

 //  if (Ethernet.begin(mac) == 0) {
 //   Serial.println("Failed to obtaining an IP address using DHCP");
 //   while(true);
 // }


  // Start UDP
Udp.begin(localPort);

}

//################################################
//Interrutroutine
//################################################
void fMeasure() {
  static int iTempLaenge;
  float fDistTemp;
  long lPosTemp;
  static long lPosOld;
  static long lTimeOld;
  static bool bLedState;
  static float fPosOld;
  static int iCntView;



  iTempLaenge++;
  liStation = liStation + 1.0;

  fDistTemp = (liStation * fDistPerImp);
  lPosTemp = (long)(fDistTemp / 100.0);
  bLedState = !bLedState;

  digitalWrite(LED_BUILTIN , bLedState);
  if (bMessungAktiv)
  {
    
  if (lPosTemp != lPosOld)
  {
    lVelWrite = (((fDistTemp - fPosOld) * 3600) / ((micros() - lTimeOld)));
    iHoeheWrite = iMaxHoehe;
    ilStationWrite = (fDistTemp / 1000.0);

    aStation[iPointer][iIndex] = (fDistTemp / 1000.0);
    aHohe[iPointer][iIndex] = iMaxHoehe;
    aVel[iPointer][iIndex] = lVelWrite;
    iPointer++;
    iCntView++;
    iMaxHoehe = 0;
    lPosOld = lPosTemp;
    lTimeOld = micros();
    fPosOld = fDistTemp;
    iIdle++;
  }
  

    if ((iPointer > 99) and (iIndex == 0) and (!bSave00))
    {
        iIndex = 1;
        iPointer = 0;
        bSave00 = true;
    }

    if ((iPointer > 99) and (iIndex == 1) and (!bSave01))
    {
        iIndex = 0;
        iPointer = 0;
        bSave01 = true;
    }
      
  }

  if (iCntView>10)
  {
   bSendView=true;
   iCntView=0;
  }

  if (iIdle>255)
{
  iIdle=0;
}

}


void fSimulation() {
  static int iTempLaenge;
  float fDistTemp;
  long lPosTemp;
  static long lPosOld;
  static long lTimeOld;
  static bool bLedState;
  static float fPosOld;
  static int iCntView;



  iTempLaenge++;
  liStation = liStation + 1.0;

  fDistTemp = (liStation * fDistPerImp);
  lPosTemp = (long)(fDistTemp / 100.0);
  bLedState = !bLedState;

  digitalWrite(LED_BUILTIN , bLedState);
  if (bMessungAktiv)
  {
    
  if (lPosTemp != lPosOld)
  {
    lVelWrite = (((fDistTemp - fPosOld) * 3600) / ((micros() - lTimeOld)));
    iHoeheWrite = iMaxHoehe;
    ilStationWrite = (fDistTemp / 1000.0);

    aStation[iPointer][iIndex] = (fDistTemp / 1000.0);
    aHohe[iPointer][iIndex] = iMaxHoehe;
    aVel[iPointer][iIndex] = lVelWrite;
    iPointer++;
    iCntView++;
    iMaxHoehe = 0;
    lPosOld = lPosTemp;
    lTimeOld = micros();
    fPosOld = fDistTemp;
    iIdle++;
  }
  

    if ((iPointer > 99) and (iIndex == 0) and (!bSave00))
    {
        iIndex = 1;
        iPointer = 0;
        bSave00 = true;
    }

    if ((iPointer > 99) and (iIndex == 1) and (!bSave01))
    {
        iIndex = 0;
        iPointer = 0;
        bSave01 = true;
    }
      
  }

  if (iCntView>0)
  {
   bSendView=true;
   iCntView=0;
  }

if (iIdle>255)
{
  iIdle=0;
}
 
}





//#################################################
// Funktion Scale
//#################################################
int fScale(int iInput)
{

  int iArray[11][11] = {{ -5, 0, 1, 2, 3, 4, 6, 8, 10, 15, 20}, {0, 10, 20, 30, 40, 50, 60, 80, 100, 150, 200}};
  int iM, iWert, i;
  for (i = 0; i < 10; i++)
  {
    if ((iInput >= iArray[i][1]) and (iInput < iArray[i + 1][1]))
    {
      iM = ((iArray[i + 1][0] - iArray[i][0]) * 100) / ((iArray[i + 1][1] - iArray[i][1]));
      iWert = (iInput * iM) + iArray[i][0];
      return iWert;
    }
  }
}




//#################################################
// Funktion Save Value To CF Card
//#################################################
int fStore(int iArray, String sFileName)
{
  long lTime;
  long lTimeWrite;
  lTime = millis();

  dataFile = SD.open(sFileName + ".csv", FILE_WRITE);
  if ((dataFile)) {
    for (int i = 0; i <= 99; i++) {
      dataFile.println(String(aStation[i][iArray]) + ";" + String(aHohe[i][iArray]) + ";" + String(aVel[i][iArray]) + ";" + "");
    }

    dataFile.close();


    lTimeWrite = millis() - lTime;
   
    return 1;
  }
  else {
    Serial.println("ErrorWrite");
    Serial.println(lTimeWrite);
    return 10;
  }
}

//#################################################
// Funktion Inital CF Card
//#################################################
boolean fStartSdCard()
{
  boolean result = false;

  pinMode(4, OUTPUT); // 4 bei UNO

  if (!SD.begin(chipSelect)) //Überprüfen ob die SD Karte gelesen werden kann
  {
    result = false;
  }

  else // Wenn ja Datei wie im Loop anlegen
  {
    File dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile)
    {
      dataFile.close();
      result = true;
    }
  }
  return result;
}


void checkUDP()
{  
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    // read the packet into packetBufffer
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
    
    
    // send "ACK" as reply
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(packetBuffer);
    Udp.endPacket();
    Serial.println(packetBuffer);
  }
 
}

// Function to send UDP packets
void sendUDP(String text)
{
    Udp.beginPacket(RecipientIP, RecipientPort);
    // Udp.write("Test");
    Udp.print(text);
    Udp.endPacket();
  
}

void fReadSdCard()
{
 char c[2048];
 String a;
 int _i=0; 
 File dataFile = SD.open("mes.csv");

Serial.println(millis());
 if (dataFile) {

    while (dataFile.available()) {
   
    for (_i=0;_i<2048;_i++)
    {
    c[_i]=dataFile.read();  
    }
    
  sendUDP(c);
    }

    dataFile.close();
    SD.remove("mes.csv");
    bSdRead=false;
  }

  // if the file isn't open, pop up an error:

  else {

    Serial.println("error opening datalog.txt");

  }
  
}
//#################################################
// Main
//#################################################

void loop()
{
  int iResult;
  int iHohe;
  int iBatterie;
  static int iStatus;
  long lRest1000;
  long lRest2000;
  long lRest5000;
  long lRest0100;
  long lRest0020;
  static bool bReadData;
  String sSend;
  unsigned long ulMillis; 
 
 ulLastTime=micros();
 
 if (lTimeUsed> lTimeUsedMax)
 {
  lTimeUsedMax=lTimeUsed;
 }

  ulMillis=millis();

  
  lRest1000=ulMillis%1000;
  lRest5000=ulMillis%5000;
  lRest0100=ulMillis%100;
  lRest0020=ulMillis%10;

  iHohe=analogRead(A0);
 

if (bMessungAktiv)
{
 if  (lRest0020==0)
 {
  fSimulation();
 }
}


  

if (iHohe>iMaxHoehe)
{
iMaxHoehe=iHohe;
}

 if ((lRest1000==0) and (lRest5000!=0))
 {
   
 
  checkUDP();

  
  if (strstr(packetBuffer, "070"))
  {
   bMessungAktiv=true;
   bKaliAktiv=false;
   bReadData=false;
  }

 if ((strstr(packetBuffer, "073") and (!bMessungAktiv)))
  {
   liStation=0.0;
   bMessungAktiv=true;
   bKaliAktiv=false;
   bReadData=false;
  }
  
  if (strstr(packetBuffer, "071"))
  { 
   bMessungAktiv=false;
   bKaliAktiv=false;
   bReadData=false;
  }

  if (strstr(packetBuffer, "072"))
  { 
   bMessungAktiv=false;
   bKaliAktiv=true;
   bReadData=false;   
  }
 

 if (strstr(packetBuffer, "074"))
  { 
   bSdRead=true;   
   bMessungAktiv=false;
   bKaliAktiv=false;
   packetBuffer[0]="";
   packetBuffer[1]="";
   packetBuffer[2]="";
   packetBuffer[3]="";
  }
  
 }

 if ((lRest5000==0) and (!bKaliAktiv))
 {
  iBatterie=analogRead(A1);
 sendUDP("#STAT#"+String(bMessungAktiv)+"#"+String(lTimeUsedMax)+"#"+String(iBatterie));
   lTimeUsedMax=0;
 }

 if ((lRest0100==0) and (bKaliAktiv))
 {
 iBatterie=analogRead(A1);
 sendUDP("#KAL#"+String(iHohe)+"#"+String(lTimeUsedMax)+"#"+String(iBatterie));
   lTimeUsedMax=0;
 }

 

 

if (bSendView)
{
sSend= String(iIdle)+";"+String(ilStationWrite)+";"+ String(iHoeheWrite)+";"+String(lVelWrite)+";"+String(lTimeUsedMax);
//sSend= String(iIdle)+";"+ String(iHoeheWrite)+"$"+String(lTimeUsedMax);


sendUDP(sSend);
bSendView=false;
lTimeUsedMax=0;
}


  if (bSave00)
  {
    iResult = fStore(0,"Mes");
    if (iResult = 1)
    {
      bSave00 = false;
    }
  }

  if (bSave01)
  {
    iResult = fStore(1,"Mes");
    if (iResult = 1)
    {
      bSave01 = false;
    }
  }

if (bSdRead)
{
  fReadSdCard();
}


lTimeUsed= micros()-ulLastTime;
}
