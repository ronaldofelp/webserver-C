# WebServer em C

Servidor web simples em C utilizando a API Winsock para comunicação via TCP/IP. Ele serve arquivos HTML, CSS e JavaScript.

## Pré-requisitos:

Para compilar e executar este servidor, você precisa de:
- Um ambiente Windows
- MinGW ou Microsoft Visual Studio (para compilar o código)
- Biblioteca Winsock2 (já incluída no Windows)

## Compilação e execução

### Compilar com MinGW:

```sh
gcc -o webserver webserver.c -lws2_32
```

### Executar:

```sh
./webserver
```

O servidor será iniciado e escutará na porta **3030** por padrão, mas você pode alterar, se desejar.

## Funcionamento

1. O servidor inicia e configura um socket TCP na porta 3030.
2. Aguarda conexões de clientes (navegadores ou ferramentas como `curl`).
3. Processa as requisições HTTP GET e tenta servir arquivos da pasta raiz (`WEBROOT`).
4. Retorna uma resposta HTTP apropriada:
   - `200 OK` com o arquivo solicitado.
   - `404 Not Found` se o arquivo não existir.
   - `500 Internal Server Error` para erros inesperados.

## Configurações

- A porta pode ser alterada modificando a constante `#define PORT 3030`.
- A pasta raiz dos arquivos estáticos pode ser ajustada em `#define WEBROOT "."`.

## Exemplo de Uso

1. Compile e execute o servidor.
2. Crie um arquivo `index.html` na mesma pasta do executável.
3. Acesse `http://localhost:3030/` no navegador.

Se tudo estiver correto, o conteúdo do `index.html` será exibido.

## Possíveis Melhorias

- Suporte para mais métodos HTTP (POST, PUT, DELETE).
- Melhor tratamento de erros.
- Logging aprimorado.
- Threads para lidar com múltiplas conexões simultaneamente.

## Licença

Este projeto é distribuído sob a licença MIT.

