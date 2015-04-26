#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <queue>
#include <string.h>
#include <limits>
#include <cfloat>
#include <float.h>

using namespace std;

// global variables
char* ARDUINO_PORT;
int exit_code;
pthread_mutex_t lock_exit;
int successful_pages;
int failed_pages;
int total_bytes;
bool zero_byte_times;

std::queue<float> temperatures;
float average = 0;
float highest = FLT_MIN;
float lowest = FLT_MAX;
char all_msg [200];
int size = 3600;
bool full = false;
char num_msg[200];
char message[200];
char previous [50] = "{\n\"name\": \"\0";
char after [20] = "\"\n}\n\0";
char before_average[20] = "Average: ";
char before_high[20] = "#Highest: ";
char before_low[20] = "#Lowest: ";
pthread_mutex_t lock_message;
int arduino; 
int running = 1;

void updateAverage(float, float);
void updateLowest(float, float);
void updateHighest(float, float);
char* get_information_from_request(char*, char*);
int start_server(int);
void* read_input_from_console(void*);
void* read_from_arduino(void*);

void updateQueue() {
	//std::size_t offset = 0;
	float celcius = 0;
	float current = 0;
	float previous = 0;
	//m = std::stod(&data[2], &offset);
	if (num_msg[0] != '\0' && num_msg[0] != '\n') {
		string str(num_msg);
		int index_of_c_f = str.length() - 1;
         while (num_msg[index_of_c_f] == '\n') {
          index_of_c_f--;
        }
		char unit = str.at(index_of_c_f);
		if (unit == 'C') {
			celcius = atof(num_msg);
			temperatures.push(celcius);
		} else {
			float fahrenheit = atof(num_msg);
			celcius = (fahrenheit - 32) * 5 / 9;
			temperatures.push(celcius);
		}
		if (temperatures.size() > size) {
			full = true;
			previous = temperatures.front();
			temperatures.pop();
		}
		current = celcius;
		//cout << "Now the temperature is " << current << endl;
		updateAverage(previous, current);
		updateLowest(previous, current);
		updateHighest(previous, current);
	}
}

void updateAverage(float previous, float current) {
	//string str(num_msg);
	if (num_msg[0] != '\0') {
		if (temperatures.size() < size) {

			float total = average * (temperatures.size() - 1) + current;
			average = total / temperatures.size();
		} else {
			if (full) {
				float total = average * size + current - previous;
				average = total / size;
			} else {
				float total = average * (temperatures.size() - 1) + current;
				average = total / temperatures.size();
			}
		}
	}
	// cout << "The average now is "<< average << endl;
}

void updateLowest(float previous, float current) {
	if (num_msg[0] != '\0') {
		if (temperatures.size() < size) {
			if (current < lowest) lowest = current;
		} else {
			if (full) {
				if (previous != lowest) {
					if (current < lowest) lowest = current;
				} else {
					lowest = FLT_MAX;
					std::queue<float> newTemperatures;
					while (temperatures.size() > 0) {
						float first = temperatures.front();
						if (first < lowest) lowest = first;
						temperatures.pop();
						newTemperatures.push(first);
					}
					temperatures = newTemperatures;
				}
			} else {
				if (current < lowest) lowest = current;
			}
		}
	}
	// cout << "The lowest now is " << lowest << endl;
}

void updateHighest(float previous, float current) {
	if (num_msg[0] != '\0') {
		if (temperatures.size() < size) {
			if (current > highest) highest = current;
		} else {
			if (full) {
				if (previous != highest) {
					if (current > highest) highest = current;
				} else {
					highest = FLT_MIN;
					std::queue<float> newTemperatures;
					while (temperatures.size() > 0) {
						float first = temperatures.front();
						if (first > highest) highest = first;
						temperatures.pop();
						newTemperatures.push(first);
					}
					temperatures = newTemperatures;
				}
			} else {
				if (current > highest) highest = current;
			}
		}
	}
	// cout << "The highest now is " << highest << endl;
}



// int main() {
// 	strcpy(all_msg, "26.67 c");
// 	updateQueue();
// 	strcpy(all_msg, "87.75 f");
// 	updateQueue();
// 	strcpy(all_msg, "32.24 c");
// 	updateQueue();
// 	strcpy(all_msg, "75.25 f");
// 	updateQueue();
// 	strcpy(all_msg, "25.55 c");
// 	updateQueue();
// 	strcpy(all_msg, "15.66 c");
// 	updateQueue();
// 	strcpy(all_msg, "22.30 c");
// 	updateQueue();
// 	// while (temperatures.size() > 0) {
// 	// 	float first = temperatures.front();
// 	// 	cout << "The first one is now "<< first << endl;
// 	// 	temperatures.pop();
// 	// }
// 	return 0;
// }



char* get_information_from_request(char* request, char* filename)
{
	int i = 5;
	int j = 0;
	while (request[i] != ' ') {
		filename[j] = request[i];
		i++;
		j++;
	}
	filename[j] = '\0';
	return filename;
}

void send_message(int fd, char* message) {
  char header [] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n\0";
  successful_pages++;
  char* content = message;
  send(fd, header, strlen(header), 0);
  total_bytes += strlen(header);
  send(fd, content, strlen(content), 0);
  total_bytes += strlen(content);
  cout << "Server sent message: " << content << endl;
}

int start_server(int PORT_NUMBER)
{
  // structs to represent the server and client
	struct sockaddr_in server_addr,client_addr;    
	
  int sock; // socket descriptor

  // 1. socket: creates a socket descriptor that you later use to make other system calls
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
  	perror("Socket");
  	exit(1);
  }
  int temp;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
  	perror("Setsockopt");
  	exit(1);
  }

  // configure the server
  server_addr.sin_port = htons(PORT_NUMBER); // specify port number
  server_addr.sin_family = AF_INET;         
  server_addr.sin_addr.s_addr = INADDR_ANY; 
  bzero(&(server_addr.sin_zero),8); 
  
  // 2. bind: use the socket and associate it with the port number
  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
  	perror("Unable to bind");
  	exit(1);
  }

  // 3. listen: indicates that we want to listen to the port to which we bound; second arg is number of allowed connections
  if (listen(sock, 1) == -1) {
  	perror("Listen");
  	exit(1);
  }
  
  // once you get here, the server is set up and about to start listening
  printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
  fflush(stdout);
  
  
  // 4. accept: wait here until we get a connection on that port
  int sin_size = sizeof(struct sockaddr_in);

  while (1) {
  	pthread_mutex_lock(&lock_exit);
  	if (exit_code == 1) {
  		pthread_mutex_unlock(&lock_exit);
      break;
  	}
  	pthread_mutex_unlock(&lock_exit);
  	
  	int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
  	printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

  	if (zero_byte_times) {
  	  zero_byte_times = false;
  	  cout << "------------------------ Arduino Disconntected" << endl;
  	  all_msg[0] = '\0';
      strcat(all_msg, previous);
      strcat(all_msg, "Arduino#Disconnected");
      strcat(all_msg, after); 
      send_message(fd, all_msg);
      close(fd);
      continue;
  	}

  // buffer to read data into
  	char request[1024];
  	
  // 5. recv: read incoming message into buffer
  	int bytes_received = recv(fd,request,1024,0);
  // null-terminate the string
  	request[bytes_received] = '\0';

    char error_header [] = "HTTP/1.1 500 Internal Server Error\n\n\0";
    char error_message [] = "Internal Server Error\0";
  	
    // get information from request
  	char* filename_tail = NULL;
  	filename_tail = (char*) malloc(sizeof(char) * 100);
  	if (filename_tail == NULL) {
  		fprintf(stderr, "%s\n", "Error: No heap available.");
  		failed_pages++;
  		
  		send(fd, error_header, strlen(error_header), 0);
  		total_bytes += strlen(error_header);
  		
  		send(fd, error_message, 21, 0);
  		total_bytes += strlen(message);
  		exit(1);
  	}

    
  	filename_tail = get_information_from_request(request, filename_tail);


  	if (strcmp(filename_tail, "favicon.ico") == 0) {
  		free(filename_tail);
  		continue;
  	}

    // case and error handling
    if (strcmp(filename_tail, "resume") == 0 || strcmp(filename_tail, "polling") == 0) {
      write(arduino, "1", 1);
      // char all_msg [200];
      all_msg[0] = '\0';
      strcat(all_msg, previous);
      if (strcmp(filename_tail, "polling") == 0) {
      	strcat(all_msg, "Polling#");
      }
      pthread_mutex_lock(&lock_message);
      int index = strlen(message) - 1;
      while (message[index] == '\n') {
        message[index] = 0;
        index--;
      }
      strcat(all_msg, message);
      pthread_mutex_unlock(&lock_message);
      strcat(all_msg, after); 
      send_message(fd, all_msg);
    } else if (strcmp(filename_tail, "pause") == 0) {
      write(arduino, "2", 1);      
      all_msg[0] = '\0';
      strcat(all_msg, previous);
      strcat(all_msg, "paused");
      strcat(all_msg, after); 
      send_message(fd, all_msg);
    } else if (strcmp(filename_tail, "celsius") == 0) {
      write(arduino, "3", 1);
      // char all_msg [200];
      all_msg[0] = '\0';
      strcat(all_msg, previous);
      pthread_mutex_lock(&lock_message);
      int index = strlen(message) - 1;
      while (message[index] == '\n') {
        message[index] = 0;
        index--;
      }
      strcat(all_msg, message);
      pthread_mutex_unlock(&lock_message);
      strcat(all_msg, after); 
      send_message(fd, all_msg);
    } else if (strcmp(filename_tail, "fahrenheit") == 0) {
      write(arduino, "4", 1);
      // char all_msg [200];
      all_msg[0] = '\0';
      strcat(all_msg, previous);
      pthread_mutex_lock(&lock_message);
      int index = strlen(message) - 1;
      while (message[index] == '\n') {
        message[index] = 0;
        index--;
      }
      strcat(all_msg, message);
      pthread_mutex_unlock(&lock_message);
      strcat(all_msg, after); 
      send_message(fd, all_msg);
    } else if (strcmp(filename_tail, "light") == 0) {
    	write(arduino, "5", 1);
    	all_msg[0] = '\0';
        strcat(all_msg, previous);
        strcat(all_msg, "fling!");
        strcat(all_msg, after); 
        send_message(fd, all_msg);
    } else if (strcmp(filename_tail, "timer") == 0) {
    	write(arduino, "6", 1);
    	all_msg[0] = '\0';
        strcat(all_msg, previous);
        strcat(all_msg, "timer!");
        strcat(all_msg, after); 
        send_message(fd, all_msg);
    } else if (strcmp(filename_tail, "statf") == 0) {
    	write(arduino, "7", 1);
    	all_msg[0] = '\0';
        strcat(all_msg, previous);
        // average
        strcat(all_msg, before_average);
        float average_f = average * 9 / 5 + 32;
        string str_average = to_string(average_f);
        str_average.erase(str_average.find_last_not_of("0") + 1);
        strcat(all_msg, str_average.c_str());
        strcat(all_msg, " F");
        // highest
        strcat(all_msg, before_high);
        float highest_f = highest * 9 / 5 + 32;
        string str_high = to_string(highest_f);
        str_high.erase(str_high.find_last_not_of("0") + 1);
        strcat(all_msg, str_high.c_str());
        strcat(all_msg, " F");
        // lowest
        strcat(all_msg, before_low);
        float lowest_f = lowest * 9 / 5 + 32;
        string str_low = to_string(lowest_f);
        str_low.erase(str_low.find_last_not_of("0") + 1);
        strcat(all_msg, str_low.c_str());
        strcat(all_msg, " F");
        
        strcat(all_msg, after); 
        send_message(fd, all_msg);
    } else if (strcmp(filename_tail, "statc") == 0) {
    	write(arduino, "8", 1);
    	all_msg[0] = '\0';
        strcat(all_msg, previous);
        // average
        strcat(all_msg, before_average);
        string str_average = to_string(average);
        str_average.erase(str_average.find_last_not_of("0") + 1);
        strcat(all_msg, str_average.c_str());
        strcat(all_msg, " C");
        // highest
        strcat(all_msg, before_high);
        string str_high = to_string(highest);
        str_high.erase(str_high.find_last_not_of("0") + 1);
        strcat(all_msg, str_high.c_str());
        strcat(all_msg, " C");
        // lowest
        strcat(all_msg, before_low);
        string str_low = to_string(lowest);
        str_low.erase(str_low.find_last_not_of("0") + 1);
        strcat(all_msg, str_low.c_str());
        strcat(all_msg, " C");

        strcat(all_msg, after); 
        send_message(fd, all_msg);
    }

  	free(filename_tail);

  // 7. close: close the socket connection
  	close(fd);
  	
  // print statistics to console
  	printf("Number of successful page requests: %d\n", successful_pages);
  	printf("Number of failed page requests: %d\n", failed_pages);
  	printf("Number of bytes sent back to the client: %d\n", total_bytes);
  	
  }

  close(sock);
  printf("Server closed connection\n");
  
  return 0;
} 

void* read_input_from_console(void* p)
{
  string input;
  while (input != "q") {
		cin >> input;
	}
  //lock the lock_exit
	pthread_mutex_lock(&lock_exit);
	exit_code = 1;
  // unlock the lock_exit
	pthread_mutex_unlock(&lock_exit);
	pthread_exit(NULL);
}

void* read_from_arduino(void* p) {
  // open the connection
  // cout << ARDUINO_PORT << endl;
  arduino = open(ARDUINO_PORT, O_RDWR);
  // cout << "arduino = " << arduino << endl;
  // if open returns -1, something went wrong!
  if (arduino == -1) return NULL;
  bool is_garbage = true;

  // then configure it
  struct termios options;
  tcgetattr(arduino, &options);
  cfsetispeed(&options, 9600);
  cfsetospeed(&options, 9600);
  tcsetattr(arduino, TCSANOW, &options);

  // a flag to check if one message is complete
  int flag = 0;

  // buf for incomplete string from arduino
  char buf[100];
  buf[0] = '\0';

  // start to build one complete message
  pthread_mutex_lock(&lock_message);
  while (1) {
    pthread_mutex_trylock(&lock_message);
     int bytes_read = read(arduino, buf, 100);
     if (bytes_read == -1) {
     	zero_byte_times = true;
     }
     if (bytes_read != 0) {
        buf[bytes_read] = '\0';

        if (flag) {
          message[0] = '\0';
          flag = 0;
        }
        strcat(message, buf);
        if (buf[bytes_read - 1] == '\n') {
          flag = 1;
          if (!is_garbage) {
          	strcpy(num_msg, message);
          	updateQueue();
          	num_msg[0] = '\0';
          }         
          pthread_mutex_unlock(&lock_message);
          pthread_mutex_lock(&lock_exit);
          if (exit_code == 1) {
            pthread_mutex_unlock(&lock_exit);
            if (pthread_mutex_trylock(&lock_message) == 0) {
              pthread_mutex_unlock(&lock_message);
            }
            break;
          }
          pthread_mutex_unlock(&lock_exit);
          is_garbage = false;
        }
     }
  }
  // if exit_code != 1, unlock lock_exit
  pthread_mutex_unlock(&lock_exit);
  return 0;
}


int main(int argc, char *argv[])
{
// check the number of arguments
	if (argc != 2)
	{
		cout << "\nUsage: server [port_number]\n";
		exit(0);
	}

	exit_code = 0;
	zero_byte_times = false;
	successful_pages = 0;
	failed_pages = 0;
	total_bytes = 0;
	num_msg[0] = '\0';
	int PORT_NUMBER = atoi(argv[1]);
  // ARDUINO_PORT = argv[2];
	ARDUINO_PORT = "/dev/cu.usbmodem1431";

	pthread_mutex_init(&lock_exit, NULL);
  pthread_mutex_init(&lock_message, NULL);
	pthread_t read_thread;
  pthread_t t1;

	pthread_create(&read_thread, NULL, &read_input_from_console, NULL);
  pthread_create(&t1, NULL, &read_from_arduino, NULL);
	start_server(PORT_NUMBER);
	pthread_join(read_thread, NULL);
  pthread_join(t1, NULL);

}
