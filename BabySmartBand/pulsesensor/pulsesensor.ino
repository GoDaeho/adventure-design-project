#include <SoftwareSerial.h>
SoftwareSerial transmitter(7, 8); //아두이노핀은 tx=7, rx=8
int sensPin = A0;     
int Signal;                        
int i = 0;
int t = 0;
float sum = 0;
float sum1 = 0;
float avg;
float avg1;


// The SetUp Function:
void setup() {
   Serial.begin(9600); // Set's up Serial Communication at certain speed.
   transmitter.begin(9600);  

// The Main Loop Function
void loop() {

  Signal = analogRead (sensPin);  //심박수 센서 값 읽기
  if(i<100){
    sum += Signal;    // 10초동안 100번의 심박수로 평균값 계산
  }
  else if(i==100){
    avg = sum/100;
  }
  else{
    if(t!=20){    //평균값 계산이 끝나면 20초동안 20번의 심박수로 평균값 계산
      sum1 += Signal;
      t = t+1;
    }
    else if(t==20){
      avg1 = sum1/20;  //기존 평균값과 20초 동안의 평균값을 비교하여 30bpm이상 차이가 나면 위험상태를 어플로 보내기
      if((avg1-avg)>100 || (avg1-avg)<-100){
        Serial.println("20초 동안 평균 bpm");
        Serial.print(avg1);
        Serial.println("상태: 위험");
        transmitter.print(1); //앱으로 1을 보낸다
        sum1 = 0;
        t = 0;
      }
      else{          // 기존 평균값과 20초 동안의 평균값의 차이가 크지 않으면, 20초동안의 평균값을 기존 평균값에 대입
        avg = avg1;
        sum1 = 0;
        t = 0;
        Serial.println("상태: 이상 없음");
        transmitter.print(0);
      }
    }
  }
  i = i+1;
  Serial.println(Signal);   
  Serial.print("평균: ");
  Serial.println(avg);
  Serial.println(i);
  Serial.println(t);
  if(i<100){   //10초 동안 평균값을 구할 땐 0.1초에 한번씩 심박수 값 받기
    delay(100);
  }
  else{        // 기준 평균값을 구한 이후에는 1초에 한번씩 심박수 값 받기
    delay(1000);
  }
}
