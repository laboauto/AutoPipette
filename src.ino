/* Reference: DroneBot Workshop  https://www.youtube.com/watch?v=0qwrnUeSpYQ
Stepper Motor test
17HS08-1004S with STSPIN220
17HS4401 with A4988
*/ 


/* Motor codes  connect A4988 pins  
    blue -> 1B
    red -> 1A
    green -> 2A
    black -> 2B
*/

//Define constants

//Connection to A4988
const int dirPin = 2;  //Direction:  HIGH -> CCW & Pull ,LOW -> CW & Push
const int stepPin = 3; //Step: HIGH -> rotate, LOW -> stop

//limit switch pin
const int limit_pull_Pin = 7;
const int limit_push_Pin = 4;

//Parameters about pressure-sensor
const int pressurePin = A0;
// const int pressurePin_500 = A1; 
int i = 0; //センサの出力値の平均を算出する際の変数．
int j = 0;
const int n = 10;//平均するデータ数
int value[n]; //センサーの値を格納する配列

//Motor steps per rotation
const int  STEPS_PER_REV = 200; //step angle = 1.80 deg
int state = 1; //1:押してない&1段階目, 2:2段階目, 3:3段階目

const int DEBOUNCE_TIME_MS = 800; // デバウンス時間をミリ秒単位で設定
const int threshold_1 = 1000; //1段階目から2段階目に遷移する閾値
const int threshold_2 = 2000; //2段階目から3段階目に遷移する閾値
const int hys = 200; //閾値のヒステリシス．センサーにより適宜変更する

const int threshold_1_high = threshold_1 + hys;
const int threshold_1_low = threshold_1 - hys;
const int threshold_2_high = threshold_2 + hys;
const int threshold_2_low = threshold_2 - hys;

void resetPosition(int direction, int switchPin);
void rotateOneStep(int delaytime = 1000);
void measurePressureSensor();
int checkThreshold(int value);

void setup()
{
    //setup the pins as Outputs
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(pressurePin, INPUT);
    pinMode(limit_push_Pin, INPUT_PULLUP);
    pinMode(limit_pull_Pin, INPUT_PULLUP);
    Serial.begin(9600);
}

void loop()
{
resetPosition(HIGH, limit_pull_Pin);
delay(100);

while(1){
    //試料吸い上げスタートするか．0を押すとスタート．
    Serial.println("Press 0");
    while (1) {
        if (Serial.available() > 0) { // 受信データがあるか確認
            if (Serial.read() == '0') { // 受信データが'0'ならばループを抜ける
                break;}}
        delay(10); // ここで少し待機する
    }

    //試料吸い上げスタート．
    Serial.println("Pressing down...");
        digitalWrite(dirPin, LOW); //push
    while(1){
        rotateOneStep(1000);
        measurePressureSensor();
        if(state == 2){
            break; //2段階目まで押したらストップ
        }
        // delay(10);
    }
    //吸い上げる
    Serial.println("aspirating...");
    digitalWrite(dirPin, HIGH); //pull
    while(digitalRead(limit_pull_Pin) == LOW){
        rotateOneStep(800);
        measurePressureSensor();
        // delay(10);
    }
    Serial.println("Sample aspirating complete");
    Serial.write('3');
    //吸い上げ完了


    //試料押し出しするかどうか．1を押すとスタート
    Serial.println("Press 1");
    while (1) {
        if (Serial.available() > 0) { // 受信データがあるか確認
            if (Serial.read() == '1') { // 受信データが'1'ならばループを抜ける
                break;}}
        delay(10); // ここで少し待機する
    }

    //試料押し出しスタート
    Serial.println("Dispensing...");
        digitalWrite(dirPin, LOW); //push
    while(1){
        rotateOneStep(1000);
        measurePressureSensor();
        if(state == 3){
            break; //3段階目まで押したらストップ
        }
        // delay(10);
    }
    Serial.println("Sample dispensing complete");
    Serial.write('4');
    //押し出し終了


    //チップ外すかどうか．2を押すとスタート
    Serial.println("Press 2");
    while (1) {
        if (Serial.available() > 0) { // 受信データがあるか確認
            if (Serial.read() == '2') { // 受信データが'2'ならばループを抜ける
                break;}}
        delay(10); // ここで少し待機する
    }

    //チップ取り外しスタート
    Serial.println("Ejecting...");
    digitalWrite(dirPin, LOW); //push
    while(digitalRead(limit_push_Pin) == LOW){
        rotateOneStep(1000);
        measurePressureSensor();
        // delay(10);
    }
    Serial.println("Tip ejecting complete");
    Serial.write('5');
    //チップ取り外し完了


    //初期位置に戻す．
    resetPosition(HIGH, limit_pull_Pin);
}
}


void resetPosition(int direction, int switchPin) {
  digitalWrite(dirPin, direction); // 回転方向を設定

  // 指定されたスイッチが押されるまで回転を続ける
    while(digitalRead(switchPin) == LOW) {
        rotateOneStep(800);
    }
    state = 1;
    for ( int j = 0; j < n; j++)
    {value[j] = 0;}
    
}

void rotateOneStep(int delaytime = 1000){
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(delaytime);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(delaytime);
}


void measurePressureSensor(){
    value[i] = analogRead(pressurePin);
    i++;
    if(i == n-1){
        int sum_value = 0;
        for(int x = 0;x < n;x++){
            sum_value += value[x];
        }
        i = 0;
        Serial.println(sum_value);
            checkThreshold(sum_value);    
    }
}


int checkThreshold(int value) {
    unsigned long currentTime = millis();
    static unsigned long lastDebounceTime = 0;

    unsigned long debounceElapsedTime = currentTime - lastDebounceTime;

    if (value > threshold_1_high && state == 1) {
        if (debounceElapsedTime >= DEBOUNCE_TIME_MS) {
            lastDebounceTime = currentTime; // 状態を変更した時点の時刻をデバウンス時刻として記録
        state = 2;
        Serial.println("state have changed to 2 from 1");
        }
    } else if (value < threshold_1_low && state == 2) {
        if (debounceElapsedTime >= DEBOUNCE_TIME_MS) {
            lastDebounceTime = currentTime; // 状態を変更した時点の時刻をデバウンス時刻として記録
        state = 1;
        Serial.println("state have changed to 1 from 2");
        }
    } else if (value > threshold_2_high && state == 2) {
        if (debounceElapsedTime >= DEBOUNCE_TIME_MS) {
            lastDebounceTime = currentTime; // 状態を変更した時点の時刻をデバウンス時刻として記録
        state = 3;
        Serial.println("state have changed to 3 from 2");
        }
    } else if (value < threshold_2_low && state == 3) {
        if (debounceElapsedTime >= DEBOUNCE_TIME_MS) {
            lastDebounceTime = currentTime; // 状態を変更した時点の時刻をデバウンス時刻として記録
        state = 2;
        Serial.println("state have changed to 2 from 3");
        }
    }
    return state;
}



