#include <iostream>
#include <boost/asio.hpp>
#include "/usr/include/mysql/mysql.h"

using namespace boost::asio;
struct LoginStruct
{
    std::array<char, 25> id;
    std::array<char, 25> pwd;
};

struct RegiStruct
{
    std::array<char, 25> id;
    std::array<char, 25> pwd;
    std::array<char, 50> NickName;
};


#define DB_HOST "127.0.0.1"
#define DB_USER "admin"
#define DB_PWD "1eodnek1"
#define DB_NAME "TPSUser"
#define TB_NAME "UserTable"

LoginStruct Login_deserialize(const std::vector<char>& buffer);
RegiStruct Regi_deserialize(const std::vector<char>& buffer);
bool CheckAlreayJoin(std::array<char, 25> id);



int main() {
    try {
        io_context io_context;

        // 소켓 생성
        ip::tcp::acceptor acceptor(io_context, ip::tcp::endpoint(ip::tcp::v4(), 8888));

        std::cout << "DB Server started. Listening on port 8888..." << std::endl;

        for (;;) {
            ip::tcp::socket socket(io_context);
            acceptor.accept(socket);

            // 클라이언트 정보 획득
            ip::tcp::endpoint clientEndpoint = socket.remote_endpoint();
            std::string clientIP = clientEndpoint.address().to_string();
            unsigned short clientPort = clientEndpoint.port();

            std::cout << "Client connected from " << clientIP << ":" << clientPort << std::endl;

            try {
                for (;;) {
                    // 데이터를 받아서 다시 클라이언트에게 보냄

                    std::vector<char> receivedData(sizeof(RegiStruct));
                    read(socket, buffer(receivedData));
                    RegiStruct receivedStruct = Regi_deserialize(receivedData);


                    // 클라이언트가 보낸 메시지 출력
                    std::cout << "message from " << clientIP << ":" << clientPort << ": " << receivedStruct.id.data() << ","
                        << receivedStruct.pwd.data() << "," << receivedStruct.NickName.data() << std::endl;

                    

                    //true이면 데이터가 없으므로 가입 할 수 있다.
                    //false이면 연결이 안되거나 이미 가입정보가 있어 가입이 안된다.
                    if(CheckAlreayJoin(receivedStruct.id))
                    {

                    }

                      

                    std::string signal = "Success";
                    // 클라이언트에게 메시지 다시 전송
                    socket.write_some(buffer(signal));
                }
            }
            catch (std::exception& e) {
                // 에러 발생 시 처리
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
#pragma region serialize
LoginStruct Login_deserialize(const std::vector<char>& buffer) {
    LoginStruct result;
    memcpy(&result, buffer.data(), sizeof(result));
    return result;
}


RegiStruct Regi_deserialize(const std::vector<char>& buffer) {
    RegiStruct result;
    memcpy(&result, buffer.data(), sizeof(result));
    return result;
}
#pragma endregion serialize

bool CheckAlreayJoin(std::array<char, 25> id)
{
    
    MYSQL* connection =NULL,conn;
    MYSQL_RES* sql_result;
    MYSQL_ROW sql_row;
    int query_stat;

   mysql_init(&conn);


   connection =mysql_real_connect(&conn,DB_HOST,DB_USER,DB_PWD,DB_NAME,3306,(char*)NULL,0);
    if(connection==NULL)
    {
        std::cout<<"mysql connect error : "<<mysql_error(&conn)<< std::endl;
        return false;
    }


    // 특정 id를 사용하여 데이터 조회
    std::string query = "SELECT * FROM UserTable WHERE id = '";
    query += string(id.data(), id.size()) + "'";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "mysql_query() failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return false;
    }

    res = mysql_store_result(conn);

    // 결과 출력
    if (res) {
         std:cout << "Already Join ID" << std::endl;
        mysql_close(conn);
        return false;
    }



    std:cout << "No Join ID" << std::endl;

    // MySQL 연결 해제
    mysql_close(conn);
    

    return true;
}