#define RENEW_INTERVAL  30000
#define RETRY_INTERVAL  5000

TCPClient client;
byte server[] = { 192, 168, 1, 100 }; // Google

// IO
int led = D0; 
int waitpin_C = D7; 
int waitpin_NO = D5; 
int waitpin_NC = D3; 

// Globals
int state = 0;
int wait = 0;

int tries = 0;
int store = 0;
int hash = 0;

void setup()
{
  Serial1.begin(9600);
  
  Serial1.println("Thermometer sketch");

  pinMode(led, OUTPUT);  
  // Button resistorless
  pinMode(waitpin_C, INPUT );  
  pinMode(waitpin_NO, OUTPUT );  
  pinMode(waitpin_NC, OUTPUT );
  digitalWrite(waitpin_NO, LOW);
  digitalWrite(waitpin_NC, HIGH);

  state = 0;
  wait = RETRY_INTERVAL;
  // Connecting
}

void loop()
{
    int i=0;
    if(!digitalRead(waitpin_C)){
        // Skip mode
        char buffer[32];
        sprintf(buffer,"skip state: %d\r\n",state);
        Serial1.print(buffer);
        digitalWrite(led, HIGH);   // Turn ON the LED       
        delay(100);
        return;
    }else{
        digitalWrite(led, LOW);   // Turn ON the LED  
    }

    static char command[16];
    delay(10);
    switch(state){
        case 0:
            // Waiting for next time
            wait-=10;
            if(wait<0){
                wait = 0;
                state = 1;
            }
            break;
        case 1:
            // Connecting
            Serial1.println("connecting");
            if (client.connect(server, 80)){
                Serial1.println("connected");
                state++;
            }else{
                Serial1.println("connection failed state 1");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;
        case 2:
            // Requesting
            for(i=0; i<sizeof(command); i++) command[i] = 0;
            tries = 0;
            store = 0;
            hash = 0;
            if(client.connected()){
                client.println("GET / HTTP/1.0\r\n\r\n");
                wait = 10;
                state = 3;
            }else{
                Serial1.println("connection lost state 2");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;
        case 3:
            // Receiving
            if(client.connected()){
                if (client.available() > 0) 
                {
                    char c = client.read();
                    if(c=='>') hash = 1;
                    if(c=='<') store = 1;
                    if(store){
                        command[i++] = c;
                    }
                    Serial1.print(c);
                    Serial1.flush();
                    /*if(tries++ > 1000){
                        Serial1.println("connection lost");
                        wait = 5000;
                        state = 0;
                        client.stop();
                        break;
                    }
                    delay(1);*/
                }else{
                    Serial1.println("no data state 3");  
                    delay(100);
                }
                if(hash){
                    Serial1.println();
                    state = 4;
                }
            }else{
                Serial1.println("connection lost state 3");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;
        case 4:
            // Disconnecting
            if(client.connected()){
                client.stop();
                Serial1.println("connection closed state 4");
                wait = RENEW_INTERVAL;
                state = 5;
            }else{
                Serial1.println("connection closed by server state 4");
                wait = RENEW_INTERVAL;
                state = 5; 
            }
            break;
        case 5:
            // Parse
            if(client.connected()){
                client.stop();
                Serial1.println("I've got this:");
                for(int i=0; i<sizeof(command); i++) Serial1.write(command[i]);
                // Control display
                int code = (command[1]*1000)+(command[2]*100)+(command[3]*10)+(command[4]*1);
                int temp = atoi(&command[6]);
                char tempString[10]; //Used for sprintf
                Serial1.println();
                Serial1.write('v');
                sprintf(tempString, "%4d", temp); //Convert deciSecond into a string that is right adjusted
                Serial1.print(tempString);
                
                wait = 5000;
                state = 0;
            }
            break;
            
    }
}