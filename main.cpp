#include "windows.h"
#include <queue>

#include "EventMessage.h"

wchar_t *char2tchar(const char *s) {
	size_t size = strlen(s) + 1;
	wchar_t *wbuf = new wchar_t[size];
	size_t outSize;
	mbstowcs_s(&outSize, wbuf, size, s, size - 1);
	return wbuf;
}

char *tchar2char(const wchar_t *t) {
	auto len = wcslen(t) + 1;
	auto s = new char[len];
	size_t sz;
	wcstombs_s(&sz, s, len, t, len);
	return s;
}

volatile bool threadRuning = false;
volatile bool exitFlag = false;

typedef std::queue< EventMessage *> MessageQueue;
MessageQueue *messageQueue = 0;

volatile HANDLE messageQueue_mutex = 0;

void readFolder(std::string folder, std::vector<std::string> &folderList) {

	WIN32_FIND_DATA ffd;
	// auto tb = char2tchar(folder.c_str());
	auto hFind = FindFirstFile(folder.c_str(), &ffd);
	// delete tb;

	if (INVALID_HANDLE_VALUE == hFind) {
		return;
	}

	do {
		if ( (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			// char *filename = tchar2char(ffd.cFileName);
			// delete filename;
			folderList.push_back(std::string(ffd.cFileName));
		}
	} while (FindNextFile(hFind, &ffd) != 0);

}


struct WriterParams {
	std::string folder;
};

DWORD WINAPI ThreadFunction(LPVOID lpParam) {

	char b[1024];
	auto params = (WriterParams *)lpParam;

	threadRuning = true;
	OutputDebugStringA("[*] start thread\n");

	std::vector<std::string> folderList;
	sprintf_s(b, "%s/*.eventlog", params->folder.c_str());
	readFolder(b, folderList);

	int index = 1;
	for (auto i = folderList.begin(); i != folderList.end(); i++) {
		auto file = *i;
		auto n = file.substr(0, file.length() - 9);
		auto num = std::stoll(n);
		if (num > index) index = num;
		// OutputDebugStringA(b);
	}

	sprintf_s(b, "%s/%d.eventlog", params->folder.c_str(), index);
	int fd;
	auto err = _sopen_s(&fd, b, _O_RDWR | _O_CREAT | _O_APPEND | _O_BINARY, _SH_DENYNO, _S_IWRITE);
	if (err) {
		OutputDebugStringA("[E] open\n");
	}

	_lseeki64(fd, 0, SEEK_END);
	auto filesize = _telli64(fd);

	int block_size = 100;

	// 

	while (true) {

		if (exitFlag && messageQueue->size() == 0) break;

		auto dwWaitResult = WaitForSingleObject(messageQueue_mutex, INFINITE);
		if (WAIT_OBJECT_0 != dwWaitResult) {
			OutputDebugStringA("messageQueue_mutex WaitForSingleObject problem\n");
		}

		auto size = messageQueue->size();
		auto block = size;
		if (size > block_size) size = block_size;
		while(size) {
			auto m = messageQueue->front();
			messageQueue->pop();
			size--;
		}
		
		ReleaseMutex(messageQueue_mutex);

		sprintf_s(b, "[*] writed %d messages\n", (int)block);
		OutputDebugStringA(b);


		Sleep(10);
	}

	OutputDebugStringA("[*] end thread\n");
	threadRuning = false;
	return 0;
}

int main() {

	OutputDebugStringA("[*] start\n");

	messageQueue = new MessageQueue();
	messageQueue_mutex = CreateMutex(NULL, FALSE, NULL);

	WriterParams params = { std::string("c:/work/events") };

	auto ht_ = CreateThread(NULL, 0, ThreadFunction, &params, 0, NULL);
	Sleep(1);

	int count = 1000;
	while (count) {

		auto m = new EventMessage;

		auto dwWaitResult = WaitForSingleObject(messageQueue_mutex, INFINITE);
		if (WAIT_OBJECT_0 != dwWaitResult) {
			OutputDebugStringA("messageQueue_mutex WaitForSingleObject problem\n");
		}
		messageQueue->push(m);
		ReleaseMutex(messageQueue_mutex);

		// Sleep(1);

		count--;
	}

	OutputDebugStringA("[*] queue filled\n");

	exitFlag = true;

	OutputDebugStringA("[*] wait for thread\n");

	while (true) {
		if (!threadRuning) break;
		Sleep(5);
	}

	OutputDebugStringA("[*] end\n");
}