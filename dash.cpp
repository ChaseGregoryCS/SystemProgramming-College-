#include <string.h>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <algorithm>

using namespace std;

typedef pair<pid_t, char*> VecTask;

void ExecuteBuiltins(vector<string> cmds, passwd * user, string& prompt, int pass, vector<VecTask> & processes);

static bool debug = false;
static bool logs = false;
static bool startup = false;
static int lFile;

enum PromptOptions{
	DEF_PROMPT = 0x00,
	ENV_PROMPT = 0x01,
	BASE_NAME = 0x02,
	HOST_NAME = 0x04,
	USER_NAME = 0x08
};

enum Redirect{
	PIPE = 0x02,
	REDIRECT_TO = 0x04,
	REDIRECT_FROM = 0x08,
	BACKGROUND = 0x16,
	NADA = 0x00
};

void KillTasks(pid_t whoToKill, vector<VecTask> & processes){
	if (debug){
		cout << "KillTask(): Called" << endl;
	}
	int i = 0;
	if (!whoToKill and processes.size() > 0){
		for(int iter = 0; iter < processes.size(); ++iter){
			kill(processes[iter].first, SIGKILL);
		}
		processes.clear();
	}else if (processes.size() > 0){
		vector<VecTask>::iterator it = processes.begin();
		while(it != processes.end()){
			cout << processes.size() << endl;
			if(it->first == whoToKill){
				processes.erase(it);
				cout << processes.size() << endl;
				kill(whoToKill, SIGKILL);
				it = processes.begin();
			}
			else it++;
		}
		

	}else{ cout << "SHOULDN'T HOSE A CLEAN BACKGROUND" << endl;}
	return;
}

void Tasks(vector<VecTask> & children){
      	if (debug){
		cout << "VecTask(): Called" << endl;
	}
	int i=0;

	cout << "Current VecTask: " << endl;
	cout << "\t(PID::Task Name) " << endl;
	if (children.size() > 0) {
	    for(i = 0; i < children.size()-1; i++) {
		cout << "\t(" << children[i].first << "::" << children[i].second << ")";
	    }

	    if (i < children.size()) {
		cout << "\t(" << children[i].first << "::" << children[i].second << ")";
	    }
	} else {
	    cout << "\tEMPTY " << endl;
	}
	cout << endl;
	return;
}

/* Tokenizes the arguments allows us to use the incoming input as a command
 * Pre: Commands have been entered
 * Post: A vector of strings containing individualized commands and arguments returned
 */
void GetCmd(string input, vector<string> & cmds){
	if (debug || logs){
		string outpt = "GetCmd(): Called";
		if (debug)
			cout << outpt << endl;
		if (logs){}
		//	fputs(outpt.c_str(), pFile);
	}

	int i, j;
	string temp = "";
	string theCmd = "";
	bool isString = false;
		//take out double white spaces 
	for(i = 0; i < input.length(); ++i){
		if(!isspace(input[i]) || (temp.length() > 0 && temp[temp.length() -1] != ' ')){	
		  //if the char at 'i' is a space, the length of reName is greater than 0
		  //and the char at reName 'i' - 1 is a space, push the char at
		  //'i' back on reName.
			temp += input[i];
		}
	}
	//take out the last char if it is a space
	if(temp[temp.length() -1] == ' ')
		temp = temp.substr(0,temp.length() -1);
	
	//cout << temp<< endl;
	temp += " ";

	for(i =0; i < temp.length(); i++){
		if (temp[i] == '\"'){
			i++;
			while (temp[i] != '\"'){
				theCmd += temp[i];
				i++;
			}
			cmds.push_back(theCmd);
		}else if (!isspace(temp[i])){
			theCmd += temp[i];
		}else{
			cmds.push_back(theCmd);
			theCmd = "";
		}
	}
	
return;
}

/*
 * Makes sure that all commands entered are valid
 * Pre: GetCmd called
 * Post: a bool indicating valid input/ string
 */
bool ValidCmd(string cmd){
  	if (debug){
		cout << "ValidCmd(): Called" << endl;
	}
	int i = 0;
	bool rValue = true;
	if (cmd.length() == 0){
		rValue = false;
	}
	return rValue;
}


/* Defines the grammar of the shell
 * Pre: commands have already been tokanized
 * Post: an integer corresponding to the builtin returned, -2 if not valid and -1 if executable command
 */
int GetCmdInt(vector<string> & cmds){
	if (debug){
		cout << "GetCmdInt(): Called" << endl;
	}

	int intCmd;
	string cmd = cmds[0];

	if (cmd == "UID")
		intCmd = 0;
	else if(cmd == "PID")
		intCmd = 1;
	else if(cmd == "PPID")
		intCmd = 2;
	else if(cmd == "NAME")
		intCmd = 3;
	else if(cmd == "WD")
		intCmd = 4;
	else if(cmd == "HD")
		intCmd = 5;
	else if(cmd == "INFO")
		intCmd = 6;
	else if(cmd == "HOME")
		intCmd = 7;
	else if(cmd == "UP")
		intCmd = 8;
	else if(cmd == "PROMPT")
		intCmd = 9;
	else if(cmd == "RUN")
		intCmd = 10;
	else if(cmd == "DOWN")
		intCmd = 11;
	else if(cmd == "GO")
		intCmd = 12;
	else if(cmd == "SET")
		intCmd = 13;
	else if(cmd == "EXIT")
		intCmd = 14;
	else if(cmd == "PLOP")
		intCmd = 15;
	else if(cmd == "TASKS")
		intCmd = 16;
	else if(cmd == "HOSE")
		intCmd = 17;
	else if(ValidCmd(cmd))
		intCmd = -1;
	else
		intCmd = -2;
return intCmd;  
}

/* Checks to see if the command entered is a builtin
 * Pre: CmdSelect was called 
 * Post: returns bool if the input is a builtin
 */
bool NotBuiltin(vector<string> & cmds){
	if (debug){
		cout << "NotBuiltin(): Called" << endl;
	}

	bool flag = false;
	for (int i = 0; i < cmds.size(); i++){
		if (GetCmdInt(cmds) == -1)
			flag = true;
	}
	
return flag;
}

/* Prints the UID
 * Pre: UID builtin was entered
 * Post: UID ouptputted
 */
void PrintUID(passwd * user){
	if (debug){
		cout << "PrintUID(): Called" << endl;
	}

	cout <<"The UID is: " << user->pw_uid<< endl;
return;
}

/* Prints the PID
 * Pre: PID builtin was entered
 * Post: PID ouptputted
 */
void PrintPID(passwd * user){
	if (debug){
		cout << "PrintPID(): Called" << endl;
	}

	cout <<"The PID is: " << getpid() << endl;
return;
}

/* Prints the PPID
 * Pre: PPID builtin was entered
 * Post: PPID ouptputted
 */
void PrintPPID(passwd * user){
	if (debug){
		cout << "PrintPPID(): Called" << endl;
	}

	cout << "The PPID is: "<< getppid() << endl;
return;
}

/* Gets the User's name
 * Pre: NAME builtin was entered
 * Post: return user's name
 */
string GetName(passwd * user){
	if (debug){
		cout << "GetName(): Called" << endl;
	}

return user->pw_name;
}

/* Gets the User's WD
 * Pre: functions that requre the WD was called
 * Post: return user's WD
 */
string GetWD(){
	if (debug){
		cout << "GetWD(): Called" << endl;
	}

	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
return (string)cwd;
}

/* Prints the User's HD
 * Pre: HD builtin was called
 * Post: prints the user's WD
 */
void PrintHD(passwd * user){
	if (debug){
		cout << "PrintHD(): Called" << endl;
	}

	cout <<"The home directory is: " <<  user->pw_dir << endl;
return;
}

/* Prints the User's basic INFO
 * Pre: INFO builtin was called
 * Post: prints the user's INFO
 */
void PrintInfo(passwd * user){
	if (debug){
		cout << "PrintInfo(): Called" << endl;
	}

	PrintUID(user);
	PrintPID(user);
	PrintPPID(user);
	cout << "Your name is: " << GetName(user) << endl;
	cout << "The Working directory is: " << GetWD() << endl;
	PrintHD(user);
return;
}

/*
 * Goes up a directory
 * Pre: UP was called
 * Post: goes up a directory in the file system. Returns the new Working directory
 */
string UpDir(passwd * user){
	if (debug){
		cout << "UpDir(): Called" << endl;
	}

	string wd = GetWD();
 	int i = wd.length();

	while (wd[i-1] != '/'){
		wd.erase(wd.begin()+i - 1);
		--i;
	}
return wd;
}

/* Gets the User's Host Name
 * Pre: PROMPT 5 and up were called
 * Post: return user's HostName
 */
string GetHostName(){
	if (debug){
		cout << "GetHostName(): Called" << endl;
	}

	string hostName;
	char name[1024];
	name[1023] ='\0';
	gethostname(name, 1023);
	int i = 0;
	while (name[i] != '.' and name[i] != '\0'){
		hostName += name[i];
		i++;
	}
return hostName;
}

/* Gets the User's Base Name
 * Pre: PROMPT (multiples of 2) and up were called
 * Post: return user's base name
 */
string GetBaseName(passwd * user){
	if (debug){
		cout << "GetBaseName(): Called" << endl;
	}

	string wd = GetWD();
	string reverseDir = "";
	string currDir = "";
 	int i = wd.length();
	while (wd[i-1] != '/'){
		reverseDir += wd[i-1];
		--i;
	}
	i = reverseDir.length();
	while (i != 0){
		currDir.push_back( reverseDir[i-1]);
		--i;
	}
return currDir;
}

/* Sets the users prompt
 * Pre: PROMPT x was called
 * Post: Prompt as a string was returned
 */
string SetPrompt(PromptOptions n, passwd * user){
	if (debug){
		cout << "SetPrompt(): Called" << endl;
	}

	string prompt = "";
	if(n == 0){
		prompt = ">";
	}
	if (n & USER_NAME){
		prompt += GetName(user);
		prompt += ":";
	}
	if(n & HOST_NAME){
		prompt += GetHostName();
		prompt += ":";
	}
	if(n & BASE_NAME){
		prompt += GetBaseName(user);
		prompt += ":";
	}
	if(n & ENV_PROMPT){
		prompt += (string)getenv("PROMPT");
	}
return prompt += " ";
}

/* Changes the directory based on Builtin grammar
 * Pre: HOME, UP, DOWN entered
 * Post: prompt of the directory returned
 */
string ChangeDir(string cmd, string wTo, passwd * user, int n){
	if (debug){
		cout << "ChangeDir(): Called" << endl;
	}

	string prompt;
	if (cmd == "HOME"){
		if (-1 == chdir(user->pw_dir));
			cout << "HOME FAIL" << endl;
	}else if (cmd == "UP"){
		if(-1 == chdir(UpDir(user).c_str())){
			cout << "UP FAIL" << endl;
		}
	}else if (cmd == "DOWN")
		chdir(wTo.c_str());
	else
		chdir(wTo.c_str());
	prompt = SetPrompt(PromptOptions(n), user);
return prompt;
}

/* Changes the vector of arguments to char** so that exec works on them
 * Pre: any process creation method called
 * Post: a char** of cmds returned
 */
char ** ConvertVecString(vector<string> cmds){
  	if (debug){
		cout << "ConvertVecString(): Called" << endl;
	}
	int i = 0;
	vector<string>::iterator j = cmds.begin();
	if(cmds.front() == "RUN") j++;
	char** toChar = new char*[cmds.size()+ 1];
	for (vector<string>::iterator iter = j; iter != cmds.end(); ++iter){
		toChar[i] = strdup(iter->c_str());
		i++;
	}
	toChar[i] = NULL;
return toChar;
}

/* Runs the cmd given
 * Pre: RUN called
 * Post: process created and ran
 */
void RunPrgm(vector<string> cmds){
	if (debug){
		cout << "RunPrgm(): Called" << endl;
	}

	pid_t childPID;
	
	char ** myRet = ConvertVecString(cmds);
	
	switch(childPID = fork()){
		case -1: cout << "Failed Fork!!" << endl;
			break;
		case 0:  cout << "Starting Process " << myRet[1] <<  endl;

			//cout << path << ' ' << myRet[1] << endl;
			if (-1 == execvp(myRet[1], myRet+1)) cout << "Invalid Process" << endl;
			break;
		default:
			wait(NULL);
			break;
	}
	delete myRet;
return;
}

/* start up process of dash
 * Pre: -s passed in as an argument upon startup
 * Post: returns the input from the file
 */
string StartUp(int& rcOpen, FILE * rc, string input){
  	if (debug){
		cout << "StartUp(): Called" << endl;
		cout << rcOpen << endl;
	}

	char line[256];
	if(rcOpen == 1){
		rcOpen = 2;
		fgets(line, sizeof(line), rc);
		if(line != NULL)
			input = line;
	}else if (rcOpen < 10){
		if(NULL != fgets(line, 256, rc)){
			if (strlen(line) > 1){
				line[strlen(line) -1] = '\0';
			}
		rcOpen = 2;
		}else rcOpen ++;
		if (isalnum(line[0])){
			input = line;
		}else input = "";
	}else{
		fclose(rc);
		++rcOpen;
		startup = false;

	}
return input;
}

/* writes input and output to a temp log
 * Pre: -l passed in as an argument upon startup
 * Post: output and input written to a log
 */
void WriteToLogs(string prompt, string input){
  	if (debug){
		cout << "WriteToLogs(): Called" << endl;
	}
	char * buffer = NULL;
	buffer = new char[input.length()]; 

	if (-1 == write(lFile, prompt.c_str() , prompt.length())) cout << "write failed" << endl;
	
	strcpy(buffer, input.c_str());
	if (-1 == write(lFile, buffer, sizeof(buffer)+ 1)) cout << "write failed" << endl;
	
	if (-1 == write(lFile, "\n",1)) cout << "write failed" << endl;
	
	delete buffer;
return;
}

/* Prompts the user and handles input
 * Pre: startup of dash
 * Post: returns the user input
 */
string IOPrompt(string prompt, int& rcOpen, FILE * rc){
	if (debug){
		cout << "IOPrompt(): Called" << endl;
	}
	
	int i = 0;
	string input = "";
	
	if (startup){
		input = StartUp(rcOpen, rc, input);
	}else{
		cout << prompt;
		if(!getline(cin, input))
		  exit(3);
	}
	
	if(logs){
		WriteToLogs(prompt, input);
	}
	
return input;
}


/* checks the vector for any special grammar IE pipe
 * Pre: non builtin entered
 * Post: returns true if the cmd has special grammar
 */
Redirect HasRedirect(vector<string> cmds){
  	if (debug){
		cout << "HasRedirect(): Called" << endl;
	}
	Redirect rValue = NADA;
	for (int i = 0; i < cmds.size(); ++i){
		if(cmds[i] == "|") rValue = PIPE;
		else if(cmds[i] == "<") rValue = REDIRECT_FROM;
		else if(cmds[i] == ">") rValue = REDIRECT_TO;
		else if(cmds[i] == "&") rValue = BACKGROUND;
	}	
return rValue;
}


/* gets the exicutable path
 * Pre: process command entered
 * Post: returns the path to execute from
 */
string GetPath(string path, int& i, char ** myRet ){
    	if (debug){
		cout << "GetPath(): Called" << endl;
	}
	string temp = "";
	int j = i;

	while (j < path.length()){
		if( path[j] == ':'){
			++j;
			i = j;
			temp += "/"; 
			temp+= *myRet;  
			j = path.length();
		}else if (path[j] == '.'){
			++j;
			temp += '.';
			i = j;
			temp += "/"; 
			temp+= *myRet;  
			j = path.length();
		}else{
			temp += path[j];
			++j;
		}
	}
	return temp;
}

/* Gets the first part of of the command before the special grammar
 * Pre: special grammar command entered
 * Post:returns where we left off in the search
 */
int getExecPrgm(char** execPrgm, char** myRet){
  	if (debug){
		cout << "getExecPrgm(): Called" << endl;
	}
	int i = 0;
	int j = 0;
	string temp = "";
	while(i < sizeof(myRet)){
		if(myRet[i] != NULL){
			temp += strdup(myRet[i]);
			
			if((temp != "<") and (temp != ">") and (temp != "|")){
				execPrgm[j] = strdup(temp.c_str());
				temp = "";
				j++;
			}else i = sizeof(myRet);
		}else i = sizeof(myRet);
		i++;
	}
	execPrgm[j] = '\0';

	return j++;
}

/* Pipes processA to processB
 * Pre: special grammar command entered
 * Post: executes a pipe between processes
 */
void PipedCmd(string path, char** myRet){
	if (debug){
		cout << "PipedCmd(): Called" << endl;
	}
	string temp = "";
	int pipefd[2];
	char ** processA = new char*[sizeof(myRet)];
	char ** processB = new char*[sizeof(myRet)];
	int i = getExecPrgm(processA, myRet);
	int j = 0;
	i++;
	cout << sizeof(myRet) << " " << i << endl;
	while(myRet[i]){
		processB[j] = strdup(myRet[i]);
// 		cout << "processB: " << processB[j] << endl;
		j++;
		i++;
	}
// 	cout << "Meh: " << endl;
	if(!fork()){
		pipe(pipefd);
		if(!fork){
			close(STDOUT_FILENO);
			close(pipefd[STDIN_FILENO]);
			dup2(pipefd[STDOUT_FILENO], STDOUT_FILENO);
			
			temp = GetPath(path, i, processA);
			while(i < path.length()){
				if (-1 != execv(temp.c_str(), processA)) i = path.length();
				perror("execv");
				temp = GetPath(path, i, processA);
			}
		}else{
			close(pipefd[STDOUT_FILENO]);
			dup2(pipefd[STDIN_FILENO], STDIN_FILENO);
			
			temp = GetPath(path, i, processB);
			while(i < path.length()){
				if (-1 != execv(temp.c_str(), processB)) i = path.length();
				perror("execv");
				temp = GetPath(path, i, processB);
			}
		}
	}else
		wait(NULL);
	return;
}

/* gets the file we are writing to and reading from
 * Pre: special grammar command entered
 * Post: returns the filename
 */
string getFile(char** myRet, int i){
  	if (debug){
		cout << "getFile(): Called" << endl;
	}
	string file;
	if(myRet[i+1])
		return file += myRet[i + 1];
	else cout << "Syntax error" << endl;
	return "";
}

/* gets and executes a process for input from a file
 * Pre: special grammar command entered
 * Post: executes program with input from a file
 */
void RedirectFrom(string path, char** myRet){
    	if (debug){
		cout << "RedirectFrom(): Called" << endl;
	}
	string temp = "";
	char** execPrgm = new char*[sizeof(myRet)+ 1];
	int i = getExecPrgm(execPrgm, myRet);
	execPrgm[sizeof(myRet) + 1] = NULL;
	string file = getFile(myRet, i);
// 	cout << "file: " << file << endl;

	int fd;
	
	
	pid_t childPID;
	childPID = fork();
	
	if(!childPID){
		close(STDIN_FILENO);
		fd = open(file.c_str(), O_RDONLY);
		if(fd != STDIN_FILENO) dup2(fd, STDIN_FILENO);
		
		temp = GetPath(path, i, execPrgm);
		while(i < path.length()){
			if (-1 != execv(temp.c_str(), execPrgm)) i = path.length();
			perror("execv");
			temp = GetPath(path, i, execPrgm);
		}
		
		exit(1);
	}else{
// 		close(STDIN_FILENO);
		wait(NULL);
		
	}	
	//delete execPrgm;
	return;
}

/* gets and executes a process for input from a file
 * Pre: special grammar command entered
 * Post: executes program with input from a file
 */
void RedirectTo(string path, char** myRet){
     	if (debug){
		cout << "RedirectTo(): Called" << endl;
	}
	string temp = "";
	char** execPrgm = new char*[sizeof(myRet)+ 1];
	int i = getExecPrgm(execPrgm, myRet);
	execPrgm[sizeof(myRet) + 1] = NULL;
	string file = getFile(myRet, i);
	int fd;
	i = 0;
	
	pid_t childPID;
	childPID = fork();
	
	if(!childPID){
		close(STDOUT_FILENO);
		fd = open(file.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if(fd != STDOUT_FILENO) dup2(fd, STDOUT_FILENO);
		temp = GetPath(path, i, execPrgm);
		while(i < path.length()){
			if (-1 != execv(temp.c_str(), execPrgm)) i = path.length();
			perror("execv");
			temp = GetPath(path, i, execPrgm);
		}
		exit(1);
		
	}else{
// 		close(STDIN_FILENO);
		wait(NULL);
		
	}	
	return;
}

/* gets and executes a process in the background
 * Pre: special grammar command entered
 * Post: executes program in the background
 */
void ExecuteBckgrnd(string path, char ** myRet, vector<VecTask> & children){
	if (debug){
		cout << "ExecuteBckgrnd(): Called" << endl;
	}
	string temp = "";
	int i = 0;
	int j;

	j = children.size();
	pid_t pIDTemp;
	pIDTemp = fork();
	
	if(pIDTemp){
		cout << "ME: " << getpid() << endl;
		children.push_back(VecTask(pIDTemp,myRet[0]));

	}else{
		temp = GetPath(path, i, myRet);
		while(i <= path.length()){
			if (-1 != execv(temp.c_str(), myRet)) i = path.length();
			//perror("execv");
			temp = GetPath(path, i, myRet);
		}
		//cout << "Execution of " << *myRet << " Failed" << endl;
 		exit(1);
	}

	return;
}

/* Executes a command without special grammar
 * Pre: process command entered
 * Post:executes process
 */
void ExecuteStdCmd(string path, char** myRet){
	if (debug){
		cout << "ExecuteStdCmd(): Called" << endl;
	}
	string temp = "";
	int i = 0;
	pid_t childPID;
	switch(childPID = fork()){
		case -1:cout << "Failed Fork!!" << endl;
			temp = "";
			break;
		case 0: 
			//temp = GetPath(path, i, myRet);
			while(i <= path.length()){
				cout << temp << endl;
				if (-1 != execv(temp.c_str(), myRet)) i = path.length();
				//perror("execv");
				temp = GetPath(path, i, myRet);
			}
			//exit(3);
		default:wait(NULL);
			temp = "";

			break;
		}
}


/* interface that selects the commands from specific grammar
 * Pre: special grammar command entered
 * Post: makes function call corresponding to the grammar entered.
 */
void ExecuteNonBuiltin(vector<string> cmds, vector<VecTask> & processes){
       	if (debug){
		cout << "ExecuteNonBuiltin(): Called" << endl;
	}
	string path = getenv("DASH_PATH");
	
	char ** myRet = ConvertVecString(cmds);	
	Redirect haveDirect = HasRedirect(cmds);
// 	cout << "haveDirect: " << haveDirect << endl;
	
	if(haveDirect == NADA){
		ExecuteStdCmd(path, myRet);
	}else if(haveDirect == REDIRECT_FROM){
		RedirectFrom(path, myRet);
	}else if(haveDirect == REDIRECT_TO){
		RedirectTo(path, myRet);
	}else if(haveDirect == PIPE){
		PipedCmd(path, myRet);
	}else if (haveDirect == BACKGROUND){
		ExecuteBckgrnd(path, myRet, processes);
	}

	delete myRet;
return;
}


/* interface that selects the commands
 * Pre: N/A
 * Post: handles input and specifies which path to take 
 */
void CmdSelect(){
	if (debug){
		cout << "CmdSelect(): Called" << endl;
	}

	int rcOpen = 1;
	int pass = 0;
	string input = ""; 
	string exit = "EXIT";
	string prompt = "> ";
	FILE * rc = NULL;
	vector<string> cmds;
	vector<VecTask> processes;
	
	passwd * user = NULL;
	uid_t uid;

	uid = getuid();
	user = getpwuid(uid);
	
	if(startup){
		if (-1 == chdir(".dash")) cout << "error" << endl;
		rc = fopen("rc", "r");
		if (NULL == rc) cout << "rc error!!"<< endl;
		ChangeDir("UP", "", user, pass);
	}
	
	if (NULL == user){
		perror("getpwuid");
	}
	
	if (startup)
		input = IOPrompt(prompt, rcOpen, rc);
	else 
		input = IOPrompt(prompt, rcOpen, NULL);
	
	while (input != "EXIT"){
		GetCmd(input, cmds);
		if (NotBuiltin(cmds))
			ExecuteNonBuiltin(cmds, processes);
		else
			ExecuteBuiltins(cmds, user, prompt, pass, processes);
		
		cmds.erase(cmds.begin(), cmds.end());
		input = IOPrompt(prompt, rcOpen, rc);
	}
	if (!rc) delete rc;
return;	
}



/* Executes builtin grammar
 * Pre: Builtin cmd entered
 * Post: calles the appropriate function corresponding to the Builtin
 */
void ExecuteBuiltins(vector<string> cmds, passwd * user, string& prompt, int pass, vector<VecTask> & processes){
	if (debug){
		cout << "ExecuteBuiltins(): Called" << endl;
	}
	int cmdInt = 0;
	
	switch (GetCmdInt(cmds)){
		case -2: break;
		case 0: PrintUID(user);
			      break;
		case 1: PrintPID(user);
			      break;
		case 2: PrintPPID(user);
			      break;
		case 3: cout << GetName(user) << endl;
			      break;
		case 4: cout << GetWD() << endl;
			      break;
		case 5: PrintHD(user);
			      break;
		case 6: PrintInfo(user);
			      break;
		case 7: ChangeDir(cmds[0], "", user, pass);
			      break;
		case 8: prompt =ChangeDir(cmds[0], "", user, pass);
			      break;
		case 9: pass = atoi(cmds[1].c_str());
			      prompt = SetPrompt(PromptOptions(pass), user);
			      break;
		case 10: ExecuteNonBuiltin(cmds, processes);
			      break;
		case 11:prompt = ChangeDir(cmds[0], cmds[1], user, pass);
			      break;
		case 12: prompt = ChangeDir(cmds[0], cmds[1], user, pass);
			      break;
		case 13: setenv(cmds[1].c_str(), cmds[2].c_str(), true);
			      break;
		case 14:      break;
		case 15: cout << cmds[1] << endl;
			      break;
		case 16: Tasks(processes);
			      break;
		case 17: if(cmds.size() > 1){
				KillTasks(atoi(cmds[1].c_str()), processes);
			 }else KillTasks(0, processes);
			 break;
		default: cout << "INVALID COMMAND!" << endl;
			      break;
	}
return;
}


/* Sets the environment variables
 * Pre: startup
 * Post: environment varivbles set
 */
string SetDefEnvVar(){
	if (debug){
		cout << "SetDefEnvVar(): Called" << endl;
	}
	string path = "/bin:";
	string wd = GetWD();
	setenv("PROMPT", "%", true);
	
	path += wd + ":";
	setenv("DASH_PATH", getenv("PATH"), true);
return wd;
}

int main(int argc, char * argv[], char * envp[]){
	int i = 1;
	string wd = SetDefEnvVar();
	
	char * logName = new char;
	strcpy(logName, "LogXXXXXX");
	
	if(-1 != mkdir(".dash",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )) cout << ".dash created"  << endl;
	
	while (i  < argc){
		if (!strcmp("-s", argv[i]) or !strcmp("--start", argv[i])){
			startup = true;			
			i++;
		}else if(!strcmp("-l", argv[i]) or !strcmp("--log", argv[i])){
			logs = true;
			if(-1 != mkdir(".dash/logs",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )) cout << "Making logs" << endl;
			chdir(".dash/logs");
			lFile = mkstemp(logName);
			if( -1 == lFile) cout << "tmpFile not created" << endl;
			chdir(wd.c_str());
			i++;
		}else if(!strcmp("-d", argv[i]) or !strcmp("--debug", argv[i])){
			debug = true;
			i++;
		}
	}
	delete logName;
	CmdSelect();
return 0;
}
