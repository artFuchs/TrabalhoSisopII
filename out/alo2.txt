# TrabalhoSisopII

Trabalho para a disciplina de Sistemas Operacionais II na UFRGS.  
O trabalho trata-se de implementar uma aplicação semelhante ao Dropbox, permitindo compartilhamento e sincronização automática de arquivos entre dois dispositivos diferentes.  
A aplicação deve rodar em ambientes Unix e devemos usar a API UDP Sockets do Unix.

## Funcionalidades Básicas

 - Multiplos usuários: O servidor deve ser capaz de tratar requisições simultâneas de vários usuários.
 - Múltiplas sessões: Um usuário deve poder utilizar o serviço através de até dois dispositivos distintos simultaneamente.
 - Consistência nas estruturas de armazenamento: As estruturas de armazenamento de dados no servidor devem ser mantidas em um estado consistente.
 - Sincronização: Cada vez que um usuário modificar um arquivo contido no diretório ‘sync_dir’ em seu dispositivo, o arquivo deverá ser atualizado no servidor e no diretório ‘sync_dir’ dos demais dispositivos daquele usuário.
 - Persistência de dados no servidor: Diretórios e arquivos de usuários devem ser restabelecidos quando o servidor for reiniciado.
