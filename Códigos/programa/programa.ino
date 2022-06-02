
//Programa: Higienizador
//Autor: Eduardo Lucas Maia
//E-mail: eduardolucasmaia@hotmail.com


//Carrega as bibliotecas do sensor ultrassonico e Thread
#include <Thread.h>
#include <ThreadController.h>
#include <Ultrasonic.h>
#include <timeout.h>


//------------INICIO Variaveis que podem ser alteradas------------

//Define os pinos para o trigger e echo
#define pino_echo_mao 4
#define pino_trigger_mao 5
#define pino_echo_pe_esquerdo 6
#define pino_trigger_pe_esquerdo 7
#define pino_echo_pe_direito 8
#define pino_trigger_pe_direito 9
#define pino_rele_mao 12
#define pino_rele_pe 13

//Define os tempos em milissegundos para as acoes
int tempoMiliInicioMao = 1500;      //Tempo em milissegundos para inicio da higienização das mãos
int tempoMiliInicioPe = 2500;       //Tempo em milissegundos para inicio da higienização dos pés
int tempoMiliHigiMao = 500;         //Tempo em milissegundos da higienização (jato) das mãos
int tempoMiliHigiPe = 500;          //Tempo em milissegundos da higienização (jato) dos pés

//Define as dintancias em CM para o sensor acionar
int distanciaMaximaMao = 15;        //Distancia maxima em cm para o sensor começar a captar a presença das mãos
int distanciaMaximaPe = 15;         //Distancia maxima em cm para o sensor começar a captar a presença dos pés

//------------FIM Variaveis que podem ser alteradas------------


//Variaveis de controle
int intervaloLeituraSensores = 200; //Não alterar
int qtdAcionouMao = 0;
int qtdAcionouPeEsquerdo = 0;
int qtdAcionouPeDireito = 0;


//Inicializa timout para suspender o jato da bomba
Timeout timeoutDesligarBombaMao;
Timeout timeoutDesligarBombaPe;


//Inicializa os sensores nos pinos definidos acima
Ultrasonic ultrasonicMao(pino_trigger_mao, pino_echo_mao);
Ultrasonic ultrasonicPeEsquerdo(pino_trigger_pe_esquerdo, pino_echo_pe_esquerdo);
Ultrasonic ultrasonicPeDireito(pino_trigger_pe_direito, pino_echo_pe_direito);


//Inicializa as threads dos sensores
ThreadController controll = ThreadController();   //ThreadController que controlará todas as threads
Thread threadMao = Thread();                      //Thread da mão
Thread threadPeEsquerdo = Thread();               //Thread do pé esquerdo
Thread threadPeDireito = Thread();                //Thread do pé direito


// callback para threadMao
void threadMaoCallback(){
  //Le as informacoes do sensor em cm
  long microsec = ultrasonicMao.timing();
  float cmMsec = ultrasonicMao.convert(microsec, Ultrasonic::CM);

  if(cmMsec <= distanciaMaximaMao) {
    qtdAcionouMao++;
  } else {
    qtdAcionouMao = 0;
  }

  int conta = round(tempoMiliInicioMao / intervaloLeituraSensores);
  if(qtdAcionouMao == conta) {
    acionarMotorMao();
  }

  //Exibe informacoes no serial monitor
  Serial.print("Distancia sensor mao em cm: ");
  Serial.println(cmMsec);
}


// callback para threadPeEsquerdo
void threadPeEsquerdoCallback(){
	//Le as informacoes do sensor em cm
  long microsec = ultrasonicPeEsquerdo.timing();
  float cmMsec = ultrasonicPeEsquerdo.convert(microsec, Ultrasonic::CM);

  if(cmMsec <= distanciaMaximaPe) {
    qtdAcionouPeEsquerdo++;
  } else {
    qtdAcionouPeEsquerdo = 0;
  }

  calculoAcionarMotorPe();

  //Exibe informacoes no serial monitor
  Serial.print("Distancia sensor pe esquerdo em cm: ");
  Serial.println(cmMsec);
}


// callback para threadPeDireito
void threadPeDireitoCallback(){
  //Le as informacoes do sensor em cm
  long microsec = ultrasonicPeDireito.timing();
  float cmMsec = ultrasonicPeDireito.convert(microsec, Ultrasonic::CM);

  if(cmMsec <= distanciaMaximaPe) {
    qtdAcionouPeDireito++;
  } else {
    qtdAcionouPeDireito = 0;
  }

  calculoAcionarMotorPe();
  
  //Exibe informacoes no serial monitor
  Serial.print("Distancia sensor pe direito em cm: ");
  Serial.println(cmMsec);
}


void calculoAcionarMotorPe(){
  int conta = round(tempoMiliInicioPe / intervaloLeituraSensores);
  if((qtdAcionouPeEsquerdo == conta && qtdAcionouPeDireito >= conta) || (qtdAcionouPeEsquerdo >= conta && qtdAcionouPeDireito == conta)) {
    qtdAcionouPeEsquerdo++;
    qtdAcionouPeDireito++;
    acionarMotorPe();
  }
}


void acionarMotorMao(){
  digitalWrite(pino_rele_mao, LOW);
  timeoutDesligarBombaMao.start();
}


void acionarMotorPe(){
  digitalWrite(pino_rele_pe, LOW);
  timeoutDesligarBombaPe.start();
}


void setup(){
	Serial.begin(9600);

	pinMode(pino_rele_mao, OUTPUT);
  pinMode(pino_rele_pe, OUTPUT);

  digitalWrite(pino_rele_mao, HIGH);
  digitalWrite(pino_rele_pe, HIGH);
  
  timeoutDesligarBombaMao.prepare(tempoMiliHigiMao);
  timeoutDesligarBombaPe.prepare(tempoMiliHigiPe);
  
	// Configurando threadMao
	threadMao.onRun(threadMaoCallback);
	threadMao.setInterval(intervaloLeituraSensores);

	// Configurando threadPeEsquerdo
	threadPeEsquerdo.onRun(threadPeEsquerdoCallback);
	threadPeEsquerdo.setInterval(intervaloLeituraSensores);

  // Configurando threadPeDireito
  threadPeDireito.onRun(threadPeDireitoCallback);
  threadPeDireito.setInterval(intervaloLeituraSensores);
  
	// Adicionando threadMao, threadPeEsquerdo e threadPeDireito ao controll
	controll.add(&threadMao);
  controll.add(&threadPeEsquerdo);
  controll.add(&threadPeDireito);
}

void loop(){
	// execute o ThreadController
	// isso verificará todos os threads dentro do ThreadController,
	// se ele deve ser executado. Se sim, ele irá executá-lo;
	controll.run();

  digitalWrite(pino_rele_mao, timeoutDesligarBombaMao.time_over());
  digitalWrite(pino_rele_pe, timeoutDesligarBombaPe.time_over());
}
