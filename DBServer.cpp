#include <iostream>
#include <boost/asio.hpp>
#include <mysql/mysql.h>

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


#define DB_HOST "localhost"
#define DB_USER "admin"
#define DB_PWD "1eodnek1"
#define DB_NAME "TPSUser"



int main() {
    try {
        io_context io_context;

        // ���� ����
        ip::tcp::acceptor acceptor(io_context, ip::tcp::endpoint(ip::tcp::v4(), 8888));

        std::cout << "DB Server started. Listening on port 8888..." << std::endl;

        for (;;) {
            ip::tcp::socket socket(io_context);
            acceptor.accept(socket);

            // Ŭ���̾�Ʈ ���� ȹ��
            ip::tcp::endpoint clientEndpoint = socket.remote_endpoint();
            std::string clientIP = clientEndpoint.address().to_string();
            unsigned short clientPort = clientEndpoint.port();

            std::cout << "Client connected from " << clientIP << ":" << clientPort << std::endl;

            try {
                for (;;) {
                    // �����͸� �޾Ƽ� �ٽ� Ŭ���̾�Ʈ���� ����

                    std::vector<char> receivedData(sizeof(RegiStruct));
                    read(socket, buffer(receivedData));
                    RegiStruct receivedStruct = Regi_deserialize(receivedData);


                    // Ŭ���̾�Ʈ�� ���� �޽��� ���
                    std::cout << "message from " << clientIP << ":" << clientPort << ": " << receivedStruct.id.data() << ","
                        << receivedStruct.pwd.data() << "," << receivedStruct.NickName.data() << std::endl;

                    

                    if(CheckAlreayJoin(receivedStruct.id))
                    {

                    }
                    else
                    {

                    }

                      

                    std::string signal = "Success";
                    // Ŭ���̾�Ʈ���� �޽��� �ٽ� ����
                    socket.write_some(buffer(signal));
                }
            }
            catch (std::exception& e) {
                // ���� �߻� �� ó��
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
    
    MYSQL *conn =NULL;
    MYSQL_RES* sql_result;
    MYSQL_ROW sql_row;
    int query_stat;

   mysql_init(&conn);


    connection=mysql_real_connect(&conn,DB_HOST,DB_USER,DB_PWD,DB_NAME,3306,(char*)NULL,0);
    if(connection==NULL)
    {
        cout<<"mysql connect error : "<<mysql_error(&conn)<<endl;
        return false;
        
    }
    else{
        cout<<"mysql connect Succese "<<endl;
        return false;
    }

    
    

    return false;
}