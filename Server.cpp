#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

#define _WIN32_WINNT 0x0601


using namespace boost;
using std::cout;
using std::endl;
using std::string;


enum State {
	IDLE =0,
	WALK,
};

struct Replication
{
	float PosX, PosY, PosZ=73.0f;
	float RotZ;
	State state;
};



struct Session
{
	shared_ptr<asio::ip::tcp::socket> sock;
	asio::ip::tcp::endpoint ep;
	string id;
	int room_no = -1;
	string sbuf;
	string rbuf;
	Replication repli;
	char buf[80];

};
std::vector<string> PlayerList;

bool CompareSessions(const Session& a, const Session& b) {
	std::cout << "Comparing IDs: " << a.id << " and " << b.id << std::endl;
	return a.id < b.id;
}

class Server
{
	asio::io_service ios;
	shared_ptr<asio::io_service::work> work;
	asio::ip::tcp::endpoint ep;
	asio::ip::tcp::acceptor gate;
	std::vector<Session*> sessions;
	boost::thread_group threadGroup;

	boost::mutex lock;
	std::vector<int> existingRooms;
	const int THREAD_SIZE = 5;

	boost::asio::steady_timer replicationTimer;

	enum Code { INVALID, SET_ID,REP,HIT };

public:
	Server(string ip_address, unsigned short port_num) :
		work(new asio::io_service::work(ios)),
		ep(asio::ip::address::from_string(ip_address), port_num),
		gate(ios, ep.protocol()),replicationTimer(ios)
	{
	
		existingRooms.push_back(0);
	}

	

	void Start()
	{

		cout << "Start Server" << endl;
		cout << "Creating Threads" << endl;
		for (int i = 0; i < THREAD_SIZE; i++)
			threadGroup.create_thread(bind(&Server::WorkerThread, this));

		// thread 잘 만들어질때까지 잠시 기다리는 부분
		this_thread::sleep_for(chrono::milliseconds(100));
		cout << "Threads Created" << endl;

		boost::asio::socket_base::reuse_address option(true);
		gate.set_option(option);
		ios.post(bind(&Server::OpenGate, this));
		replicationTimer.expires_from_now(std::chrono::milliseconds(100)); // 초기 타이머 만료 시간 설정
		replicationTimer.async_wait(boost::bind(&Server::SendReplication, this)); // 타이머 시작
		threadGroup.join_all();
	}

private:
	void WorkerThread()
	{
		lock.lock();
		cout << "[" << boost::this_thread::get_id() << "]" << " Thread Start" << endl;
		lock.unlock();

		ios.run();

		lock.lock();
		cout << "[" << boost::this_thread::get_id() << "]" << " Thread End" << endl;
		lock.unlock();
	}

	void OpenGate()
	{
		system::error_code ec;
		gate.bind(ep, ec);
		if (ec)
		{
			cout << "bind failed: " << ec.message() << endl;
			return;
		}

		gate.listen();
		cout << "Gate Opened" << endl;

		StartAccept();
		cout << "[" << boost::this_thread::get_id() << "]" << " Start Accepting" << endl;
	}

	// 비동기식 Accept
	void StartAccept()
	{
		Session* session = new Session();
		shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(ios));
		session->sock = sock;
		gate.async_accept(*sock, session->ep, bind(&Server::OnAccept, this, _1, session));
	}

	void OnAccept(const system::error_code& ec, Session* session)
	{
		if (ec)
		{
			cout << "accept failed: " << ec.message() << endl;
			return;
		}

		lock.lock();
		sessions.push_back(session);
		cout << "[" << boost::this_thread::get_id() << "]" << " Client Accepted" << endl;
		lock.unlock();

		ios.post(bind(&Server::Receive, this, session));

		StartAccept();
	}

	// 동기식 Receive (쓰레드가 각각의 세션을 1:1 담당)
	void Receive(Session* session)
	{
		system::error_code ec;
		size_t size;
		size = session->sock->read_some(asio::buffer(session->buf, sizeof(session->buf)), ec);

		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] read failed: " << ec.message() << endl;
			CloseSession(session);
			return;
		}

		if (size == 0)
		{
			cout << "[" << boost::this_thread::get_id() << "] peer wants to end " << endl;
			CloseSession(session);
			return;
		}

	

		session->buf[size] = '\0';
		session->rbuf = session->buf;
		PacketManager(session);
		//cout << "[" << boost::this_thread::get_id() << "] " << session->rbuf << endl;

		Receive(session);
	}



	void PacketManager(Session* session)
	{
		// :~ 라는 특수(?)메세지를 보내왔을 경우 처리
		if (session->buf[0] == ':')
		{
			Code code = TranslatePacket(session->rbuf);

			switch (code)
			{
			case Code::SET_ID:
				SetID(session);
				break;
			case Code::REP:
				ReplicationUpdate(session);
				break;
			case Code::HIT:
				PlayerHit(session);
				break;
			case Code::INVALID:
				session->sbuf = "유효하지 않은 명령어 입니다";
				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
				break;
			}
		}
		else  // :~ 라는 특수메세지가 아니고 그냥 채팅일 경우
		{
			if (session->id.length() != 0) // id length가 0인 경우는 id를 아직 등록하지 않은 경우
			{
				string temp = "[" + session->id + "]:" + session->rbuf;
				SendAll(session, session->room_no, temp, false);
			}
			else
			{
				session->sbuf = "Register your ID first through :set";
				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
			}
		}
	}

	Code TranslatePacket(string message)
	{
		string temp = message.substr(0, sizeof(":set ") - 1);
		// :set 일 경우
		if (temp.compare(":set ") == 0)
		{
			return Code::SET_ID;
		}
		temp = message.substr(0, sizeof(":rep ") - 1);
		if (temp.compare(":rep ") == 0)
		{
			return Code::REP;
		}
		temp = message.substr(0, sizeof(":hit ") - 1);
		if (temp.compare(":hit ") == 0)
		{
			return Code::HIT;
		}
		return Code::INVALID;
	}
	void ReplicationUpdate(Session* session)
	{
		string temp = session->rbuf.substr(sizeof(":rep ") - 1, session->rbuf.length());

		Replication data = deserializeReplication(temp);
		
		session->repli.PosX = data.PosX;
		session->repli.PosY = data.PosY;
		session->repli.PosZ = data.PosZ;
		session->repli.RotZ = data.RotZ;
		session->repli.state = data.state;

	}
	void PlayerHit(Session* session)
	{
		string temp = session->rbuf.substr(sizeof(":hit ") - 1, session->rbuf.length());
		
		std::istringstream ss(temp);
		std::string token;

		int index;
		int damage;
		// 쉼표로 구분된 각 부분을 추출하고 정수로 변환
		if (std::getline(ss, token, ','))
		{
			index = std::stoi(token);
		}

		if (std::getline(ss, token, ','))
		{
			damage = std::stoi(token);
		}

		string message = ":hit " + std::to_string(damage);
		sessions[index]->sbuf = message;
		sessions[index]->sock->async_write_some(asio::buffer(sessions[index]->sbuf),
			bind(&Server::OnSend, this, _1));
		

	}

	void SetID(Session* session)
	{
		string temp = session->rbuf.substr(sizeof(":set ") - 1, session->rbuf.length());
		// 중복된 아이디인지 체크
		for (int i = 0; i < sessions.size(); i++)
		{
			if (temp.compare(sessions[i]->id) == 0)
			{
				session->sbuf = "set falied: [" + temp + "]는 이미 사용중인 아이디 입니다";
				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
				return;
			}
		}

		session->id = temp;
		session->sbuf = "set [" + temp + "] success!";

		//새로 들어온 플레이어를 플레이어 목록에 저장
		PlayerList.push_back(temp);
		//플레이어 목록을 직렬화
		string ListSerial =":add "+ serializeStringArray(PlayerList);

		std::sort(sessions.begin(), sessions.end(), [](const Session* a, const Session* b) {
			return a->id < b->id;
			});

		session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));




		if (session->room_no == -1)
		{
			session->room_no = 0;
			SendAll(session, 0, ListSerial, true);
		}
		else
		{
			SendAll(session, session->room_no, "[" + session->id + "] 님이 아이디를 변경하였습니다", false);
		}
	}


	void SendAll(Session* session, int room_no, string message, bool sendToSenderAsWell)
	{
		// 같은 방에 있는 다른 모든 클라이언트들에게 보낸다
		for (int i = 0; i < sessions.size(); i++)
		{
			if ((session->sock != sessions[i]->sock) && (room_no == sessions[i]->room_no))
			{
				sessions[i]->sbuf = message;
				sessions[i]->sock->async_write_some(asio::buffer(sessions[i]->sbuf),
					bind(&Server::OnSend, this, _1));
			}
		}

		// 메세지를 보내온 클라이언트에게도 보낸다
		if (sendToSenderAsWell)
		{
			session->sbuf = message;
			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
		}
	}

	void OnSend(const system::error_code& ec)
	{
		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] async_write_some failed: " << ec.message() << endl;
			return;
		}
	}
	
	bool IsTheMessageInNumbers(string message)
	{
		const char* cTemp = message.c_str();

		// 메세지 내용(방번호)이 정수가 아닐 경우
		for (int i = 0; i < message.length(); i++)
		{
			if (cTemp[i] < '0' || cTemp[i] > '9')
			{
				return false;
			}
		}

		return true;
	}


	void CloseSession(Session* session)
	{
		if (session->room_no != -1)
		{
			SendAll(session, 0, ":exit "+session->id, false);
		}
		// if session ends, close and erase
		for (int i = 0; i < sessions.size(); i++)
		{
			if (sessions[i]->sock == session->sock)
			{
				lock.lock();
				sessions.erase(sessions.begin() + i);
				lock.unlock();
				break;
			}
		}

		string temp = session->id;
		session->sock->close();

		auto it = std::remove(PlayerList.begin(), PlayerList.end(), temp);
		PlayerList.erase(it, PlayerList.end());

		delete session;
	}

	void SendReplication() {

		if (sessions.size()>=2) {

			std::vector<Replication> temp;
			for (int i = 0; i < sessions.size(); i++)
			{
				temp.push_back(sessions[i]->repli);
			}
			string Data = ":rep " + serializeReplicationArray(temp);
			for (int i = 0; i < sessions.size(); i++)
			{
				if (0 == sessions[i]->room_no)
				{
						sessions[i]->sbuf = Data;
						sessions[i]->sock->async_write_some(asio::buffer(sessions[i]->sbuf),
							bind(&Server::OnSend, this, _1));
				}
			}
	}
		// 다음 타이머 만료 시간 설정
		replicationTimer.expires_at(replicationTimer.expires_at() + std::chrono::milliseconds(100));
		// 다음 타이머 시작
		replicationTimer.async_wait(boost::bind(&Server::SendReplication, this));
	}

	std::string serializeReplication(const Replication& rep) {
		std::stringstream ss;

		// PosX, PosY, PosZ, RotZ를 문자열로 쓰기
		ss << rep.PosX << ' ' << rep.PosY << ' ' << rep.PosZ << ' ' << rep.RotZ << ' ';

		// State 열거체 값을 int로 변환하여 쓰기
		ss << static_cast<int>(rep.state);

		return ss.str();
	}

	//플레이어 목록 직렬화
	std::string serializeStringArray(const std::vector<std::string>& strArray) {
		std::ostringstream oss;

		// 배열의 크기를 먼저 저장
		oss << strArray.size() << " ";

		// 각 문자열을 차례대로 저장
		for (const auto& str : strArray) {
			// 문자열 길이를 먼저 저장
			oss << str.length() << " " << str << " ";
		}

		return oss.str();
	}

	std::string serializeReplicationArray(const std::vector<Replication>& repArray) {
		std::stringstream ss;

		for (const auto& replication : repArray) {
			ss << serializeReplication(replication) << '\n'; // 각 FReplication 객체를 직렬화하여 쓰기
		}

		return ss.str();
	}
	Replication deserializeReplication(const std::string& data) {
		Replication replication;
		std::stringstream ss(data);
		// PosX, PosY, PosZ, RotZ를 문자열에서 읽어오기
		ss >> replication.PosX >> replication.PosY >> replication.PosZ >> replication.RotZ;

		// State 열거체 값을 int로 읽어와 다시 열거체 값으로 변환
		int stateValue;
		ss >> stateValue;
		replication.state = static_cast<State>(stateValue);

		return replication;
	}

};


int main()
{

	Server serv(asio::ip::address_v4::any().to_string(), 7777);
	serv.Start();
	
	return 0;
}