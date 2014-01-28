#define RENEW_INTERVAL      60*1000      // 30 secs
#define RETRY_INTERVAL      10*1000      // 10 secs
#define RESPONSE_INTERVAL   1*1000       // 1 sec

TCPClient client;
byte server[] = { 192, 168, 1, 100 }; // Google

// IO
int led = D0; 
int waitpin_C = D7; 
int waitpin_NO = D5; 
int waitpin_NC = D3; 
int displayReset = A0;

// Globals
volatile int state = 0;
volatile int wait = 0;
volatile int loopwait = 10;

volatile int tries = 0;
volatile int store = 0;
volatile int hash = 0;

volatile char command[32];
volatile int command_i=0;


// Displays float with one decimal
int display(float fx){
    int ix, idec, iint;
    char tempString[10]; //Used for sprintf
    // Lets see if we can display this number
    if(fx < -999.0 || fx > 999.0)
        return -1;  // We cannot
    
    // Round
    if(fx<0)
        ix = fx*-10;
    else
        ix = fx*10;
    
    // Reset display
    Serial1.write(0x81);
    
    // Write display
    if(fx<0){
        sprintf(tempString, "-%3d", ix); //Convert deciSecond into a string that is right adjusted
    }else{
        sprintf(tempString, "%4d", ix); //Convert deciSecond into a string that is right adjusted
    }
    delay(10);
    Serial1.print(tempString);
    delay(10);
    // Enable dot
    Serial1.write(0x77);
    Serial1.write(0x04);  // Enable dot 2
    
    
    return 0;
    
}

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
    
    // Display reset
    pinMode(displayReset, OUTPUT);
    digitalWrite(displayReset, LOW);
    delay(10);
    digitalWrite(displayReset, HIGH);
    
    state = 0;
    wait = RETRY_INTERVAL;
    
    display(1.2);
    
    // Connecting
}

void loop()
{
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
            tries = 0;
            store = 0;
            hash = 0;
            command_i = 0;
            for(int i=0; i<sizeof(command); i++) command[i] = 0;
            if(client.connected()){
                client.println("GET // HTTP/1.0\r\n\r\n");
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
                    if(c =='>') hash = 1;
                    // If first char of data found, start storing the string
                    if(c =='<') store = 1;
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
        {
            float temp = 0.0;
            int code = 0;
            // Parse data read
            client.stop();
            Serial1.println("I've got this:");
            for(int i=0; i<sizeof(command); i++) Serial1.write(command[i]);
            Serial1.println("");
            
            // Control display
            //int code = (command[1]*1000)+(command[2]*100)+(command[3]*10)+(command[4]*1);
            //float temp = atoi((const char*)&command[6]) + (0.1 * atoi((const char*)&command[8]));
            sscanf((const char*)command,"<%d;%f>",&code,&temp);
            
            Serial1.print("Code: ");Serial1.println(code);
            Serial1.print("Temp: ");Serial1.println(temp);
  
            display(temp);
        
            
            /*
            char tempString[10]; //Used for sprintf
            Serial1.println();
            Serial1.write('v');
            sprintf(tempString, "%4d", temp); //Convert deciSecond into a string that is right adjusted
            Serial1.print(tempString);
            */
            wait = RENEW_INTERVAL;
            state = 0;
        }
            break;
            
    }
}
