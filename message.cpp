enum DataTypes {
	dt_null = 0,
	dt_int8,
	dt_uint8,
	dt_int16,
	dt_uint16,
	dt_int32,
	dt_uint32,
	dt_int64,
	dt_uint64,
	dt_float,
	dt_double,
	dt_string,
	dt_array,
	dt_map
};

enum MessageTypes {
	m_none = 0,
	m_player_create,
	m_player_login
};


struct EventMessagePart;
struct EventMessagePart {
	EventMessagePart *next;
	char *buffer;
	int size, used;
};

class EventMessage {

	EventMessagePart *first, *current;
	int pos;
	int size;

public:

	EventMessage() {
		current = first = new EventMessagePart{ 0, new char[256], 256, 4 };
		size = pos = 4;		
	}

	~EventMessage() {
		auto p = first;
		while (p) {
			auto n = p->next;
			delete p->buffer;
			delete p;
			p = n;
		}
	}

	void write(int fd) {
		auto p = first;
		while (p) {

			_write(fd, p->buffer, p->used);

			p = p->next;
		}
	}

	void push_memory(void *m, unsigned int msize) {

		if (pos + msize > current->size) {
			// close current
			int psize = 256;
			if (psize < msize) {
				psize = msize;
			}
			auto p = new EventMessagePart{ 0, new char[psize], psize, 0 };
			current->next = p;
			current = p;
			pos = 0;
		}

		memcpy(current->buffer + pos, m, msize);
		pos += msize;
		current->used += msize;
		size += msize;
	}

	void push_uint8(unsigned char i) {
		push_memory(&i, 1);
	}

	void push_uint16(unsigned short i) {
		push_memory(&i, 2);
	}

	void push_uint32(unsigned long i) {
		push_memory(&i, 4);
	}

	void push_uint64(unsigned long long i) {
		push_memory(&i, 8);
	}

	void push_string(std::string &s) {
		auto sz = s.length() + 1;
		push_uint32(sz);
		push_memory((void *)s.c_str(), s.length());
		push_uint8(0);
	}

	void set_size() {		
		memcpy(first->buffer, &size, 4);
	}
};


		EventMessage *m = new EventMessage();

		m->push_uint8(dt_array);
		m->push_uint32(7);			// item count

		m->push_uint8(dt_uint64);
		m->push_uint64(timestamp);

		m->push_uint8(dt_uint16);
		m->push_uint16(messageType);
		
		m->push_uint8(dt_uint64);
		m->push_uint64(stoull(steamId->ToString()));

		m->push_uint8(dt_string);
		m->push_string(steamNameutf);
				
		m->push_uint8(dt_uint64);
		m->push_uint64(id);

		m->push_uint8(dt_string);
		m->push_string(nameutf8);

		m->push_uint8(dt_uint64);
		m->push_uint64(stoull(ip.ToString()));

		m->set_size();
		int fd = _open("d:/zh/000001.eventlog", _O_CREAT | _O_APPEND | _O_BINARY | _O_WRONLY, _S_IWRITE);
		m->write(fd);
		_close(fd);
