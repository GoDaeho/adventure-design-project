//송신부1(가속도 센서)
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"

#include <SoftwareSerial.h>
SoftwareSerial transmitter(7, 8); //아두이노핀은 tx=7, rx=8

//mpu6050 라이브러리 생성
MPU6050 mpu;

//인터럽트핀 
#define INTERRUPT_PIN 2  // use pin 2 on Arduino Uno & most boards

// 센서 상태들을 저장할 변수들
bool dmpReady = false;  
uint8_t mpuIntStatus;   
uint8_t devStatus;      
uint16_t packetSize;    
uint16_t fifoCount;     
uint8_t fifoBuffer[64]; 

//최종 값을 위한 변수들 Yaw / Pitch / Roll
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
float sum = 0;
float average = 0;
int i = 0;
int k = 0;


//인터럽트 변수
volatile bool mpuInterrupt = false;  

//인터럽트 핀에 반응하는 콜벡함수
//조금 설명이 이상하긴 한데...
//인터럽트가 발생하면 아래의 함수를 호출하게 된다.
//자세한 부분은 아래 설정에서 설명
void dmpDataReady() {
    mpuInterrupt = true;
}

int button = 3;
void setup() {
  // put your setup code here, to run once:
  pinMode(button, INPUT);
 
  Serial.begin(115200); //pc-아두이노 통신
  transmitter.begin(9600);  //아두이노-스마트폰 통신

  //I2C 셋팅 및 시작   
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties

  //MPU 6050 센서 초기화
    mpu.initialize();

    //MPU 6050 센서 초기화
    mpu.initialize();
    
    //인터럽트핀(2) 입력으로 설정
    pinMode(INTERRUPT_PIN, INPUT);

    //연결 확인
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    //DMP 초기화
    //DMP란 MPU6050내부에 있는 Digital Motion Processor
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();

    // 초기 감도 셋팅.... 일단 그대로 두고 패스
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); 

    //초기화가 잘되었다면?
    if (devStatus == 0) {
 
        //DMP 활성화
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // 여기서 인터럽트 핀과 함수를 연결시켜준다.
        //즉, 인터럽트 PIN이 LOW -> HIGH로 올라갈때 dmpDataReady 함수가 호출되는 것이다.
        Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        //준비는 끝났다!!!
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady = true;

        // FIFO 패킷 사이즈 얻어오기
        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        //에러 났을 때
        // ERROR!
        // 1 = 초기화 에러
        // 2 = DMP 업데이트 에러        
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
}

void loop() {
  // put your main code here, to run repeatedly:

   // 위에서 초기화가 잘 안됐다면 그냥 함수 종료
   if (!dmpReady) return;

   // 위에서 인터럽트와 연결된 함수에서 mpuInterrupt변수를 설정하는데
   // 이곳에선 그 변수를 기다리다가 인터럽트가 발생하면 다음으로 넘어가게 
   // 구성되어 있다.
   while (!mpuInterrupt && fifoCount < packetSize);

   // 인터럽트 변수 초기화
   mpuInterrupt = false;
   // mpu6050 상태 읽기
   mpuIntStatus = mpu.getIntStatus();

   // FIFO 버퍼 개수 얻기
   fifoCount = mpu.getFIFOCount();
    
   //  fifo가 넘쳤다면?
   if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
       mpu.resetFIFO();
       Serial.println(F("FIFO overflow!"));    
   } 
   else if (mpuIntStatus & 0x02) {
       // packetSize만큼 fifo가 들어올때까지 대기              
       while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

       // 데이터 받아 오기
       mpu.getFIFOBytes(fifoBuffer, packetSize);        
       fifoCount -= packetSize;

       //값 얻어오기
       mpu.dmpGetQuaternion(&q, fifoBuffer);
       mpu.dmpGetGravity(&gravity, &q);
       mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

       if(i < 110 && i>=10){  // 자이로 센서로 10초동안 100번의 값을 입력받아 평균값 계산(처음 10번 값은 값이 튀는 경우가 있어서 평균을 제외) 
        sum += (ypr[2] * 180/M_PI);
       }
       else{
        average = sum/100;
       }
       i += 1;

       if(i>110){  // 평균값을 구한뒤에, 평균값과 비교하여 좌 우 방향 150도 이상씩 기울어지면 시간 k를 카운트
        if((ypr[2] * 180/M_PI)+180 <= average +30 || (ypr[2] * 180/M_PI)+180 >= average +330){
         k+=1;
          }
       
        else{
        k=0;
        }
       }
       if(k>=30){ // 기울어진 상태가 30초 이상 유지되면 
        Serial.print(1);
        transmitter.print(1);  //앱으로 1을 보낸다. (위험상황)
       }
       else{
        Serial.print(0); 
        transmitter.print(0); //앱으로 0을 보낸다.
       }
    
       //출력
       Serial.print("ypr\t");
       Serial.print(ypr[0] * 180/M_PI);   //Yaw
       Serial.print("\t");
       Serial.print(ypr[1] * 180/M_PI);   //Pitch
       Serial.print("\t");
       Serial.println(ypr[2] * 180/M_PI); //Roll
       Serial.println(average);
       Serial.println(k);
       if (i <110){
        delay(100);
       }
       else{
        delay(1000);
       }
  }    
}
