# `zrok-digitaltwin.sh` — Explicação e Guia de Uso

Este documento descreve o script `zrok-digitaltwin.sh` presente na raiz do projeto. Ele explica o que o script faz, como instalar e configurar o cliente `zrok`, e como utilizá-lo no Linux para expor um dispositivo local à internet via um *reserved share* do zrok.

---

## ✅ O que o script faz

O script `zrok-digitaltwin.sh` tem como objetivo expor um dispositivo da rede local identificado pelo hostname mDNS `twinesp32.local` para acesso remoto pela internet usando o serviço [**zrok**](https://zrok.io).

### Fluxo geral:

1. Resolve o endereço IP local do hostname `twinesp32.local`.
2. Se a resolução com `getent` falhar, tenta com `avahi-resolve` (usado em distribuições Linux com suporte a mDNS).
3. Se um IP for obtido, cria ou ativa um *reserved share* com o nome `digitaltwinmaria`.
4. Inicia o compartilhamento reservado com o cliente zrok.

### Principais trechos do script:

```bash
HOSTNAME="twinesp32.local"
RESERVED_SHARE_NAME="digitaltwinmaria"
```

* Resolve o IP via:

  ```bash
  getent hosts "$HOSTNAME"
  ```

  ou, como fallback:

  ```bash
  avahi-resolve -n "$HOSTNAME"
  ```
* Cria a reserva pública:

  ```bash
  zrok reserve public -n "$RESERVED_SHARE_NAME" $IP
  ```
* Inicia o compartilhamento:

  ```bash
  zrok share reserved "$RESERVED_SHARE_NAME"
  ```

Se o IP não for resolvido, o script exibe uma mensagem de erro e finaliza com `exit 1`.

---

## 🧰 Pré-requisitos

* **Sistema operacional:** Linux.
* **Cliente `zrok` instalado** e habilitado com sua conta.
* Dispositivo acessível na rede local via mDNS como `twinesp32.local`.
* Acesso à internet para o `zrok` funcionar corretamente.
* Permissão para executar scripts e comandos no terminal.

---

## 🔧 Instalação do `zrok` (Linux)

1. Acesse o site oficial do zrok:
   👉 [https://zrok.dev](https://zrok.dev)

2. Instale o cliente usando o script automatizado:

   ```bash
   curl -fsSL https://get.zrok.dev | bash
   ```

   Ou, se preferir, baixe o binário manualmente no repositório e mova-o para `/usr/local/bin` ou outro diretório no seu `$PATH`.

3. Verifique a instalação:

   ```bash
   zrok version
   ```

---

## 🔐 Como obter sua chave (`enable key`) no zrok

O cliente `zrok` requer ativação com uma chave associada à sua conta. Para obtê-la:

1. Acesse:
   👉 **[https://api-v1.zrok.io/](https://api-v1.zrok.io/)**

2. Faça login com sua conta (GitHub, Google etc.).

3. Clique no ícone de **❓ “Getting Started”** no canto superior direito da interface.

4. Será exibido um comando semelhante a:

   ```bash
   zrok enable SUA_CHAVE
   ```

5. Copie e execute o comando no terminal:

   ```bash
   zrok enable <SUA_CHAVE>
   ```

> Pronto! Seu cliente zrok estará autorizado a criar e gerenciar compartilhamentos com sua conta.

---

## 🚀 Como usar o script `zrok-digitaltwin.sh`

1. Torne o script executável:

   ```bash
   chmod +x zrok-digitaltwin.sh
   ```

2. Execute o script na raiz do projeto:

   ```bash
   ./zrok-digitaltwin.sh
   ```

3. Saída esperada em caso de sucesso:

   ```
   🔍 Resolving IP for twinesp32.local...
   ✅ Resolved twinesp32.local to 192.168.1.42
   🚀 Launching zrok reserved share: digitaltwinmaria
   [zrok output...]
   ```

4. Caso a resolução falhe:

   ```
   ❌ Failed to resolve IP for twinesp32.local
   ```

5. Se tudo estiver correto, o terminal exibirá uma URL pública (ou via a interface web do zrok) que pode ser usada para acessar o dispositivo remotamente.

---

## ⚙️ Personalização do script

Você pode personalizar o comportamento do script editando as variáveis no início do arquivo:

```bash
HOSTNAME="twinesp32.local"
RESERVED_SHARE_NAME="digitaltwinmaria"
```

Exemplo de personalização:

* Alterar o hostname para outro dispositivo local:

  ```bash
  HOSTNAME="meudispositivo.local"
  ```
* Alterar o nome do *reserved share*:

  ```bash
  RESERVED_SHARE_NAME="meuprojeto"
  ```

Também é possível adaptar o script para aceitar argumentos via linha de comando, como:

```bash
./zrok-digitaltwin.sh --host twinesp32.local --name digitaltwinmaria
```

> Se quiser, posso implementar essa melhoria para você.

---

## 🧪 Dependências adicionais

* O script utiliza `getent` e, como fallback, `avahi-resolve` para resolver nomes mDNS.
* Certifique-se de que o pacote `avahi-utils` está instalado (caso necessário):

```bash
sudo apt update
sudo apt install -y avahi-utils
```

* Para macOS, substitua o fallback por:

```bash
dns-sd -G v4 <hostname>
```

ou

```bash
ping <hostname>.local
```

---

## 🔐 Boas práticas de segurança

* **Evite hardcode** de valores sensíveis; use variáveis de ambiente ou arquivos `.env`.
* Proteja o acesso ao compartilhamento via configurações do zrok (ACLs, autenticação etc.).
* Revogue reservas não utilizadas via painel web ou CLI.
* Mantenha seu token (`enable key`) em local seguro e privado.
