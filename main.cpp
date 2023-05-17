#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <signal.h>
#include <sstream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

const string FILENAME = "applicants.txt";
const int MAX_APPLICANTS = 100;
const int MAX_APPLICANTS_PER_DAY = 10;
const vector<string> VALID_DAYS = {"Monday",   "Tuesday", "Wednesday",
                                   "Thursday", "Friday",  "Saturday",
                                   "Sunday"};
struct Message {
  long mtype;
  char mtext[100];
  int counter;
};

struct Applicant {
  string name;
  vector<string> availableDays;
};

vector<Applicant> applicants;

bool isValidDay(const string &day) {
  return find(VALID_DAYS.begin(), VALID_DAYS.end(), day) != VALID_DAYS.end();
}

bool dayIsFull(const string &day) {
  int count = 0;
  for (const Applicant &applicant : applicants) {
    if (find(applicant.availableDays.begin(), applicant.availableDays.end(),
             day) != applicant.availableDays.end()) {
      count++;
    }
  }
  return count >= MAX_APPLICANTS_PER_DAY;
}

void printApplicantList() {
  for (Applicant applicant : applicants) {
    cout << applicant.name << ": ";
    for (string day : applicant.availableDays) {
      cout << day << " ";
    }
    cout << endl;
  }
}
bool applicantExists(const string &name) {
  for (const Applicant &applicant : applicants) {
    if (applicant.name == name) {
      return true;
    }
  }
  return false;
}
void saveToFile() {
  ofstream outFile(FILENAME, ios_base::app);
  for (Applicant applicant : applicants) {
    outFile << applicant.name;
    for (string day : applicant.availableDays) {
      outFile << " " << day;
    }
    outFile << endl;
  }
  outFile.close();
}

void loadFromFile() {
  string inputFilename;
  cout << "Enter the file name: ";
  cin >> inputFilename;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
  ifstream inFile(inputFilename);
  if (!inFile) {
    cout << "Error: Cannot open the file '" << inputFilename << "'" << endl;
    return;
  }

  string line;

  while (getline(inFile, line) && applicants.size() < MAX_APPLICANTS) {
    Applicant applicant;
    istringstream iss(line);
    iss >> applicant.name;
    string day;
    while (iss >> day) {
      if (isValidDay(day)) {
        if (!dayIsFull(day)) {
          applicant.availableDays.push_back(day);
        } else {
          cout << "Maximum number of applicants reached for " << day << endl;
        }
      } else {
        cout << "Invalid day in file: " << day << ". Skipping this entry."
             << endl;
        applicant.availableDays.clear();
        break;
      }
    }
    if (!applicant.availableDays.empty()) {
      applicants.push_back(applicant);
    }
  }

  inFile.close();
  cout << "Applicants loaded from file: " << inputFilename << endl;
}

void modifyApplicant() {
  string name;
  cout << "Enter name: ";
  cin >> name;

  bool found = false;
  for (Applicant &applicant : applicants) {
    if (applicant.name == name) {
      found = true;
      cout << "Enter new name (leave empty to keep the current name): ";
      string newName;
      cin.ignore();
      getline(cin, newName);
      if (!newName.empty()) {
        applicant.name = newName;
      }

      cout << "Enter new available days (separated by space, leave empty to "
              "keep current days): ";
      string newDays;
      getline(cin, newDays);
      if (!newDays.empty()) {
        vector<string> newAvailableDays;
        istringstream iss(newDays);
        string day;
        while (iss >> day) {
          if (!dayIsFull(day)) {
            newAvailableDays.push_back(day);
          } else {
            cout << "Maximum number of applicants reached for " << day << endl;
          }
        }
        if (!newAvailableDays.empty()) {
          applicant.availableDays = newAvailableDays;
        }
      }
      saveToFile();
      cout << "Applicant modified" << endl;
      break;
    }
  }

  if (!found) {
    cout << "Applicant not found" << endl;
  }
}

void addApplicant() {
  if (applicants.size() >= MAX_APPLICANTS) {
    cout << "Maximum number of applicants reached" << endl;
    return;
  }

  string name, days;
  cout << "Enter name: ";
  cin >> name;

  vector<string> availableDays;
  cout << "Enter available days (separated by space): ";
  cin.ignore();
  getline(cin, days);
  istringstream iss(days);
  string day;
  while (iss >> day) {
    if (isValidDay(day)) {
      availableDays.push_back(day);
    } else {
      cout << "Invalid day: " << day << ". Skipping this entry." << endl;
    }
  }
  if (availableDays.empty()) {
    cout << "No valid days were entered. Applicant not added." << endl;
    return;
  }

  bool allDaysAvailable = true;
  for (string day : availableDays) {
    vector<string> dayApplicants;
    for (Applicant applicant : applicants) {
      if (find(applicant.availableDays.begin(), applicant.availableDays.end(),
               day) != applicant.availableDays.end()) {
        dayApplicants.push_back(applicant.name);
      }
    }
    if (dayApplicants.size() >= MAX_APPLICANTS_PER_DAY) {
      cout << "Maximum number of applicants reached for " << day << " ("
           << dayApplicants.size() << "/" << MAX_APPLICANTS_PER_DAY
           << "): " << endl;
      for (string name : dayApplicants) {
        cout << name << " ";
      }
      cout << endl;
      allDaysAvailable = false;
      break;
    }
  }

  if (allDaysAvailable) {
    Applicant applicant;
    applicant.name = name;
    applicant.availableDays = availableDays;
    applicants.push_back(applicant);
    saveToFile();
    cout << "Applicant added" << endl;
  }
}

void deleteApplicant() {
  string name;
  cout << "Enter name: ";
  cin >> name;

  vector<int> indices;
  for (int i = 0; i < applicants.size(); i++) {
    if (applicants[i].name == name) {
      indices.push_back(i);
    }
  }

  if (!indices.empty()) {
    if (indices.size() > 1) {
      cout << "Found multiple applicants with the same name:" << endl;
      for (int i = 0; i < indices.size(); i++) {
        cout << i + 1 << ". " << applicants[indices[i]].name << ": ";
        for (string day : applicants[indices[i]].availableDays) {
          cout << day << " ";
        }
        cout << endl;
      }
      cout << "Enter the number of the applicant you want to delete: ";
      int index;
      cin >> index;
      if (index > 0 && index <= indices.size()) {
        applicants.erase(applicants.begin() + indices[index - 1]);
        cout << "Applicant deleted" << endl;
      } else {
        cout << "Invalid index. No applicant was deleted." << endl;
      }
    } else {
      applicants.erase(applicants.begin() + indices[0]);
      cout << "Applicant deleted" << endl;
    }
  } else {
    cout << "Applicant not found" << endl;
  }
}

void displayApplicantsByDay(const string &day) {
  int count = 0;
  for (const Applicant &applicant : applicants) {
    if (find(applicant.availableDays.begin(), applicant.availableDays.end(),
             day) != applicant.availableDays.end()) {
      count++;
    }
  }
  cout << "Number of applicants for " << day << ": " << count << endl;

  cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int send(int mqueue, const char *message_text, int counter) {
  struct Message message;
  message.mtype = 5;
  strncpy(message.mtext, message_text, sizeof(message.mtext) - 1);
  message.mtext[sizeof(message.mtext) - 1] = '\0';
  message.counter = counter;

  int status = msgsnd(mqueue, &message, sizeof(message) - sizeof(long), 0);
  if (status < 0) {
    perror("msgsnd error");
  }
  return 0;
}

int receive(int mqueue) {
  struct Message message;

  int status = msgrcv(mqueue, &message, sizeof(message) - sizeof(long), 5, 0);
  if (status < 0) {
    perror("msgrcv error");
  } else {
    cout << "The " << message.mtext << " has arrived with " << message.counter
         << " applicants\n";
    cout << "**************************************************" << endl;
  }
  return 0;
}

void handler(int signumber) {
  if (signumber == SIGUSR1) {
    cout << "Signal with number " << signumber << " has arrived\n";
  } else if (signumber == SIGUSR2) {
    cout << "Signal with number " << signumber << " has arrived\n";
  }
}

void process(const string &day) {
  int count = 0;
  for (const Applicant &applicant : applicants) {
    if (find(applicant.availableDays.begin(), applicant.availableDays.end(),
             day) != applicant.availableDays.end()) {
      count++;
    }
  }

  struct sigaction sigact;
  sigact.sa_handler = handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGUSR1, &sigact, NULL);
  sigaction(SIGUSR2, &sigact, NULL);

  int pipe_fd[2];
  if (pipe(pipe_fd) == -1) {
    perror("Pipe creation failed!\n");
    exit(1);
  }

  key_t key = ftok("applicant.txt", 1);
  if (key < 0) {
    perror("ftok error");
    exit(1);
  }

  int messg = msgget(key, 0600 | IPC_CREAT);
  if (messg < 0) {
    perror("msgget error");
    exit(1);
  }

  if (count == 0) {
    cout << "No applicant\n";
  } else if (count < 5) {
    pid_t pid = fork();
    if (pid == 0) {
      cout << "Child process 1 is created\n";
      kill(getppid(), SIGUSR1);
      close(pipe_fd[1]);
      char buffer[51];
      int bytesRead;
      while ((bytesRead = read(pipe_fd[0], buffer, 50)) > 0) {
        buffer[bytesRead] = '\0'; // null terminator
        cout << buffer << endl;
      }
      close(pipe_fd[0]);
      send(messg, "first bus", count);
      exit(1);
    } else if (pid > 0) {
      sigset_t sigset;
      sigfillset(&sigset);
      sigdelset(&sigset, SIGUSR1);
      sigdelset(&sigset, SIGUSR2);
      sigsuspend(&sigset);

      close(pipe_fd[0]);
      cout << "The parent process sent " << count
           << " applicants to the child:\n";
      for (const Applicant &applicant : applicants) {
        if (find(applicant.availableDays.begin(), applicant.availableDays.end(),
                 day) != applicant.availableDays.end()) {
          write(pipe_fd[1], applicant.name.c_str(), applicant.name.size());
          write(pipe_fd[1], "\n", 1);
        }
      }
      close(pipe_fd[1]);
      receive(messg);
      waitpid(pid, NULL, 0);
      int status = msgctl(messg, IPC_RMID, NULL);
      if (status < 0) {
        perror("msgctl error\n");
        exit(1);
      }
    } else {
      cout << "Error creating child process!\n";
    }
  } else {
    pid_t pid1 = fork();

    if (pid1 == 0) {
      cout << "Child process 1 created!\n";
      kill(getppid(), SIGUSR1);
      close(pipe_fd[1]);

      char buffer[51];
      int bytesRead;
      while ((bytesRead = read(pipe_fd[0], buffer, 50)) > 0) {
        buffer[bytesRead] = '\0';
        cout << buffer << endl;
      }
      close(pipe_fd[0]);
      send(messg, "first bus", count - (count - 5));
      exit(1);
    } else if (pid1 > 0) {
      sigset_t sigset;
      sigfillset(&sigset);
      sigdelset(&sigset, SIGUSR1);
      sigdelset(&sigset, SIGUSR2);
      sigsuspend(&sigset);

      close(pipe_fd[0]);
      cout << "The parent process sent first 5 applicants to the child:\n";
      int i = 0;
      for (const Applicant &applicant : applicants) {
        if (i >= 5)
          break;
        if (find(applicant.availableDays.begin(), applicant.availableDays.end(),
                 day) != applicant.availableDays.end()) {
          write(pipe_fd[1], applicant.name.c_str(), applicant.name.size());
          write(pipe_fd[1], "\n", 1);
          i++;
        }
      }
      close(pipe_fd[1]);
      receive(messg);
      waitpid(pid1, NULL, 0);

      // new pipe for communication with the second child
      int pipe_fd2[2];
      if (pipe(pipe_fd2) == -1) {
        perror("Pipe creation failed!\n");
        exit(1);
      }

      pid_t pid2 = fork();
      if (pid2 == 0) {
        sleep(2);
        cout << "Child process 2 created!\n";
        kill(getppid(), SIGUSR2);
        close(pipe_fd2[1]);

        char buffer[51];
        int bytesRead;
        while ((bytesRead = read(pipe_fd2[0], buffer, 50)) > 0) {
          buffer[bytesRead] = '\0';
          cout << buffer << endl;
        }
        close(pipe_fd2[0]);
        send(messg, "second bus", count - 5);
        exit(1);
      } else if (pid2 > 0) {
        sigset_t sigset;
        sigfillset(&sigset);
        sigdelset(&sigset, SIGUSR1);
        sigdelset(&sigset, SIGUSR2);
        sigsuspend(&sigset);

        close(pipe_fd2[0]);
        cout << "The parent process sent remaining " << count - 5
             << " applicants to the child:\n";
        int available_applicants = 0;
        for (const Applicant &applicant : applicants) {
          if (find(applicant.availableDays.begin(),
                   applicant.availableDays.end(),
                   day) != applicant.availableDays.end()) {
            available_applicants++;
            if (available_applicants > 5) {
              write(pipe_fd2[1], applicant.name.c_str(), applicant.name.size());
              write(pipe_fd2[1], "\n", 1);
            }
          }
        }
        close(pipe_fd2[1]);
        receive(messg);
        waitpid(pid2, NULL, 0);
        int status = msgctl(messg, IPC_RMID, NULL);
        if (status < 0) {
          perror("msgctl error");
        }
      }
    }
  }
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main() {

  int choice;
  string input, day;
  bool valid;

  do {
    cout << "1. Add applicant" << endl;
    cout << "2. Modify applicant" << endl;
    cout << "3. Delete applicant" << endl;
    cout << "4. Display all applicants" << endl;
    cout << "5. Display applicants by day" << endl;
    cout << "6. Save to file" << endl;
    cout << "7. Load from file" << endl;
    cout << "8. Process" << endl;
    cout << "9. Quit" << endl;
    cout << "Enter your choice: ";

    getline(cin, input);
    valid = true;

    for (char c : input) {
      if (!isdigit(c)) {
        valid = false;
        break;
      }
    }

    if (valid) {
      choice = stoi(input);
    } else {
      choice = -1;
      cout << "Please enter a valid number." << endl;
    }

    switch (choice) {
    case 1:
      addApplicant();
      break;
    case 2:
      modifyApplicant();
      break;
    case 3:
      deleteApplicant();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      break;
    case 4:
      printApplicantList();
      break;
    case 5:
      cout << "Enter the day: ";
      cin >> day;
      displayApplicantsByDay(day);
      break;
    case 6:
      saveToFile();
      break;
    case 7:
      loadFromFile();
      break;
    case 8:
      cout << "Enter the day: ";
      cin >> day;
      process(day);
      break;
    case 9:
      break;
    default:
      cout << "Invalid choice. Please choose a number between 1 and 8." << endl;
    }
  } while (choice != 9);

  return 0;
}
