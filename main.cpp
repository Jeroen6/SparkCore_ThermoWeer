#define RENEW_INTERVAL      300000      // 30 secs
#define RETRY_INTERVAL      10000       // 10 secs
#define RESPONSE_INTERVAL   1000        // 1 sec

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
int loopwait = 10;

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
    char command[32];
    int command_i=0;
    int i;
    if(!digitalRead(waitpin_C)){
        // Skip mode, to ensure bootloader stays available
        char buffer[32];
        sprintf(buffer,"skip state: %d\r\n",state);
        Serial1.print(buffer);
        digitalWrite(led, HIGH);   // Turn ON the LED       
        delay(100);
        return;
    }else{
        digitalWrite(led, LOW);   // Turn OFF the LED  
    }


    delay(1);
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
                state = 2;
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
                wait = RESPONSE_INTERVAL;
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
                    // Print response to serial
                    char c = client.read();
                    Serial1.print(c);
                    
                    // If last expected char found, quit reading
                    if(c=='>') hash = 1;
                    // If first char of data found, start storing the string
                    if(c=='<') store = 1;
                    if(store){
                        command[command_i++] = c;
                    }
                }else{
                    Serial1.println("nd s3 ");  
                    delay(100);
                }
                // Quit reading
                if(hash){
                    Serial1.println();
                    state = 4;
                }
            }else{
                // We lost connection
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
            // Parse data read
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
            break;
            
    }
}