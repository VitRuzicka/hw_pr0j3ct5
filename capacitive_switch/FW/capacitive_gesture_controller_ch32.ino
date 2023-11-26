#define CAP1 C4
#define CAP2 C3
#define LED D6
#define OUT D7

bool cap1LastSt = false;
bool cap2LastSt = false;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  pinMode(OUT, OUTPUT);
  pinMode(CAP1, INPUT);
  pinMode(CAP2, INPUT);
}

void loop() {
  if(digitalRead(CAP1) && !cap1LastSt){ //rising edge detect
    cap1LastSt = true;
    if(digitalRead(CAP2)){
      digitalWrite(LED, HIGH);
      digitalWrite(OUT, HIGH);
    }
  }else if(!digitalRead(CAP1) && cap1LastSt){
    cap1LastSt = false;
  }
  
  if(digitalRead(CAP2) && !cap2LastSt){ //rising edge detect
    cap2LastSt = true;
    if(digitalRead(CAP1)){
      digitalWrite(LED, LOW);
      digitalWrite(OUT, LOW);
    }
  }else if(!digitalRead(CAP2) && cap2LastSt){
    cap2LastSt = false;
  }

  delay(10);
}
