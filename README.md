# 🚀 DK Server: A Jornada do C++23 Moderno

Bem-vindo ao **DK Server**.  
Este projeto não é apenas mais um servidor web é um experimento profundo sobre o potencial do **C++23**, **Corrotinas** e **I/O Assíncrono de Baixo Nível**.

O objetivo principal:

> **Construir um servidor capaz de lidar com milhares de conexões simultâneas (C10k) sem frameworks pesados, entendendo cada byte que trafega pela rede.**

Tudo foi construído manualmente, do zero.

---

## 🛠️ O Que É Esse Projeto?

O DK Server é um servidor **HTTP/1.1** e **HTTP/2** de alta performance, completamente **assíncrono** e **não-bloqueante**.

Ele nunca fica parado esperando disco ou rede ele continua processando tudo em paralelo.

### 🔄 Filosofia Zero-Copy

A maioria dos servidores copia dados entre Kernel ⇆ Aplicação.  
O DK Server evita isso usando **Zero-Copy**, que reduz drasticamente latência.

- **Windows:** uso de `TransmitFile` e **IOCP**  
- **Linux:** arquitetura preparada para `io_uring` e `splice`

---

## 💻 Linguagens e Tecnologias

Aqui não tem mágica. Tem engenharia pura.

### **C++23**
A alma do projeto.  
Usamos corrotinas modernas (`co_await`, `co_return`) para escrever código assíncrono com clareza de código síncrono, sem “callback hell”.

### **OpenSSL**
Implementamos a camada TLS manualmente dentro do loop assíncrono.

### **Windows IOCP**
A API de I/O mais poderosa do Windows  usada como base do servidor.

### **Python**
Criamos scripts automatizados de testes (incluindo validador de HTTP/2).

### **PowerShell / Batch**
Para automação de build e criação de certificados.

---

## 🔥 As Grandes Dificuldades  
O “suor e lágrimas” do desenvolvimento

### 1. Implementar HTTP/2 + HPACK

A parte mais complexa de todo o projeto.

HTTP/2 é binário, comprimido e extremamente sensível a erros.  
Passamos horas debugando um:


#### Causa:
Nosso encoder HPACK usava índices errados da Tabela Estática.  
O navegador recebia, por exemplo, um `content-type` que virou `:status: 404`.  
E corretamente rejeitava a conexão.

#### Solução:
Criamos um **dump hexadecimal completo** dos frames para inspecionar byte a byte até corrigir a codificação.

---

### 2. TLS Assíncrono

Integrar OpenSSL com I/O assíncrono (IOCP) é um desafio enorme.

- O OpenSSL quer ler/escrever quando ele quer
- O SO só permite quando o socket está pronto

Criamos uma camada de adaptação usando **BIO pair**, permitindo que o OpenSSL funcionasse dentro do loop assíncrono.

---

### 3. Gerenciamento de Memória

Para garantir latência mínima, aplicamos uma regra:

> **Proibido alocar memória durante requisições.**

Nada de `new`, `malloc` ou qualquer alocação no caminho quente do servidor.

A solução:  
Criamos um **BufferPool** próprio, que recicla blocos de memória de forma controlada.

---

## 📊 Performance

Em testes locais (loopback), o servidor entregou:

- **≈ 36.000 req/s (pico)**  
- **≈ 23 ms de latência média com carga pesada**

Esses números foram possíveis graças a:

- `TCP_NODELAY` ativado  
- Cache agressivo de handles de arquivo  
- Estratégias zero-copy  
- Roteamento interno otimizado  

---

## 🔮 O Futuro

O DK Server já é funcional e estável, servindo páginas via HTTPS + HTTP/2.

Os próximos passos incluem:

- Suporte a multi-threading (escalonamento por múltiplos núcleos)
- Modo worker pool
- Melhor benchmark em cenários reais
- Suporte a WebSockets e streams HTTP/2
- Adição de rotas dinâmicas com corrotinas

---

## 🧡 Desenvolvido com Paixão

Criado para estudar sistemas operacionais, protocolos modernos e C++ de verdade aquele C++ “raiz”, feito na unha.


