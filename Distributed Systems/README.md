/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/
Projeto de SD

===Resumo do Projeto===
Este projeto consiste num modelo de ligação e comunicação entre múltiplos clientes e servidores através do Modelo de Dados Zookeeper. Permite que os clientes realizem várias operações sobre uma tabela de dados através dos vários servidores.

===Como Correr===
1. Abrir um terminal na pasta projeto2 e compilar o programa através do comando 'make'
2. No mesmo terminal, executar os seguintes comandos:
    zkServer.sh start
    zkCli.sh
3. Para cada servidor ou cliente que se quiser adicionar, abrir um terminal na pasta projeto2/binary (Nota: não se pode executar um cliente sem haver pelo menos um servidor ligado)
4. Para executar um novo servidor, usar o seguinte comando:
    ./table_server numero_de_porto numero_de_listas 127.0.0.1:2181 (substituir numero_de_porto por um inteiro acima de 1024 que não tenha sido ainda usado e numero_de_listas pelo número desejado de listas)
5. Para executar um novo cliente, usar o seguinte comando:
    ./table_client 127.0.0.1:2181

===Operações na Tabela===
1. put <key> <value>
    Introduz um novo par chave-valor na tabela.
2. get <key>
    Devolve o valor correspondente à chave key caso esta exista
3. del <key>
    Apaga um par chave-valor da tabela
4. size
    Devolve o número de elementos na tabela (pares)
5. getkeys
    Devolve todas as chaves presentes na tabela
6. gettable
    Devolve todos os pares chave-valor da tabela segundo a sintaxe <chave> :: <valor>
7. stats
    Devolve as estatísticas do servidor
