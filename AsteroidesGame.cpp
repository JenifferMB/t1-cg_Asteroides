#include <iostream>
#include <cmath>
#include <ctime>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <string.h>
#include <cstdlib> 
#include <unistd.h>
#include <chrono>

using namespace std;

#ifdef WIN32
#include <windows.h>
#include <glut.h>
#else
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#endif

#ifdef __linux__
#include <GL/glut.h>
#endif

#include "Ponto.h"
#include "Instancia.h"
#include "ModeloMatricial.h"
#include "Temporizador.h"
#include "ListaDeCoresRGB.h"

Temporizador T;
double AccumDeltaT = 0;
Temporizador T2;

// Personagens
const int MAX_PERSONAGENS = 500;
const int MAX_MODELOS = 50;
Instancia Personagens[MAX_PERSONAGENS];
ModeloMatricial Modelos[MAX_MODELOS];
int AREA_DE_BACKUP = 250;

// Disparos disparador
const int MAX_BALAS_DISP = 10;
int balasDispCount = 0;

// Limites lógicos da área de desenho
Ponto Min, Max;

// Geral
bool desenha = false;
int nInstancias = 0;
int nModelos = 0;
int ModeloCorrente;
int PersonagemAtual;
float angulo = 0.0;
int colisoesdisparador = 0;
bool telaCongelada = false;
string mensagemFinal = ""; 
double tempoInicialMensagem = -1.0;

struct PersonagemTemporizador {
    double tempoRestante;
};

PersonagemTemporizador Temporizadores[MAX_PERSONAGENS];
double TempoDesdeUltimaAtualizacao = 0.0;
const double INTERVALO_ATUALIZACAO = 7.0;

// Disparos
void DispararDisp(int personagemIndex);
void AtualizarBalasDisp(double dt);
void AtualizaDisparosInimigos();
void DispararInimigo(int personagemIndex);

// Geral
void ReiniciarJogo();
void ContagemVidas();
void CriaInstancias();
void AtualizaJogo();
void PrintString(string s, int posX, int posY, int cor);

// Personagens
void LimitaPersonagensTela(Instancia &personagem);
void AtualizaPersonagens(float);
void AtualizaDirecaoAleatoria(double deltaT);
void CriaPersonagem(int &i, Ponto posicao, Ponto escala, float rotacao, int idModelo, Ponto pivot, Ponto direcao, float velocidade, bool projetil, bool PersonagemPrincipal);

// Envelopes
void AtualizaEnvelope(int personagem);
void AtualizaEnvelopeBala(int balasIndex);

// Colisões
bool TestaColisao(int Objeto1, int Objeto2);
bool TestaColisaoBala(int balasIndex, int personagemIndex);
void VerificarColisoesEEstadoDoJogo();

// **********************************************************************
//
// **********************************************************************
void CarregaModelos()
{
    Modelos[0].leModelo("prometheus.txt");
    Modelos[1].leModelo("MatrizProjetil.txt");
    Modelos[2].leModelo("cinos.txt");
    Modelos[3].leModelo("coiso.txt");
    Modelos[4].leModelo("cora.txt");
    Modelos[5].leModelo("darkside.txt");
    Modelos[6].leModelo("cyberv.txt");
    Modelos[7].leModelo("redmeteor.txt");
    Modelos[8].leModelo("oiram.txt");
    Modelos[9].leModelo("solaris.txt");

    nModelos = 10;
}

// **********************************************************************
//
// **********************************************************************
void init()
{
    // Define a cor do fundo da tela (AZUL)
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

    CarregaModelos();
    CriaInstancias();
    float d = 20;
    Min = Ponto(-d, -d);
    Max = Ponto(d, d);

    srand(time(0));
}

double nFrames = 0;
double TempoTotal = 0;

// **********************************************************************
//
// **********************************************************************
void animate()
{
    double dt;
    dt = T.getDeltaT();
    AccumDeltaT += dt;
    TempoTotal += dt;
    nFrames++;

    if (AccumDeltaT > 1.0 / 30) // fixa a atualização da tela em 30
    {
        AccumDeltaT = 0;
        angulo += 2;
        AtualizarBalasDisp(dt);
        glutPostRedisplay();
    }
    if (TempoTotal > 5.0)
    {
        cout << "Tempo Acumulado: " << TempoTotal << " segundos. ";
        cout << "Nros de Frames sem desenho: " << nFrames << endl;
        cout << "FPS(sem desenho): " << nFrames / TempoTotal << endl;
        TempoTotal = 0;
        nFrames = 0;
    }
}

// **********************************************************************
//  void reshape( int w, int h )
//  trata o redimensionamento da janela OpenGL
// **********************************************************************
void reshape(int w, int h)
{
    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Define a área a ser ocupada pela área OpenGL dentro da Janela
    glViewport(0, 0, w, h);
    // Define os limites lógicos da área OpenGL dentro da Janela
    glOrtho(Min.x, Max.x, Min.y, Max.y, -10, +10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// **********************************************************************
//
// **********************************************************************
void SetaCor(int cor)
{
    defineCor(cor);
}

// **********************************************************************
//
// **********************************************************************
void DesenhaEixos()
{
    Ponto Meio;
    Meio.x = (Max.x + Min.x) / 2;
    Meio.y = (Max.y + Min.y) / 2;
    Meio.z = (Max.z + Min.z) / 2;

    glBegin(GL_LINES);
    // eixo horizontal
    glVertex2f(Min.x, Meio.y);
    glVertex2f(Max.x, Meio.y);
    // eixo vertical
    glVertex2f(Meio.x, Min.y);
    glVertex2f(Meio.x, Max.y);
    glEnd();
}

// **********************************************************************
//
// **********************************************************************
void DesenhaCelula()
{
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(0, 1);
    glVertex2f(1, 1);
    glVertex2f(1, 0);
    glEnd();
}

// **********************************************************************
//
// **********************************************************************
void DesenhaBorda()
{
    glBegin(GL_LINE_LOOP);
    glVertex2f(0, 0);
    glVertex2f(0, 1);
    glVertex2f(1, 1);
    glVertex2f(1, 0);
    glEnd();
}

// **********************************************************************
//
// **********************************************************************
void DesenhaPersonagemMatricial()
{
    ModeloMatricial MM;

    int ModeloDoPersonagem = Personagens[PersonagemAtual].IdDoModelo;
    MM = Modelos[ModeloDoPersonagem];

    glPushMatrix();
    int larg = MM.nColunas;
    int alt = MM.nLinhas;
    // cout << alt << " LINHAS e " << larg << " COLUNAS" << endl;
    for (int i = 0; i < alt; i++)
    {
        glPushMatrix();
        for (int j = 0; j < larg; j++)
        {
            int cor = MM.getColor(alt - 1 - i, j);
            if (cor != -1) // não desenha células com -1 (transparentes)
            {
                SetaCor(cor);
                DesenhaCelula();
                defineCor(Wheat);
                DesenhaBorda();
            }
            glTranslatef(1, 0, 0);
        }
        glPopMatrix();
        glTranslatef(0, 1, 0);
    }

    glPopMatrix();
}

// **********************************************************************
//
// **********************************************************************
void AtualizaEnvelopeBala(int balasIndex)
{
    Instancia &I = Personagens[balasIndex];
    ModeloMatricial MM = Modelos[I.IdDoModelo];

    Ponto centro = I.Posicao;
    float largura = MM.nColunas;
    float altura = MM.nLinhas;
    float angulo = I.Rotacao;

    Ponto A = Ponto(-largura / 2, -altura / 2);
    Ponto B = Ponto(largura / 2, -altura / 2);
    Ponto C = Ponto(largura / 2, altura / 2);
    Ponto D = Ponto(-largura / 2, altura / 2);

    A.rotacionaZ(angulo);
    B.rotacionaZ(angulo);
    C.rotacionaZ(angulo);
    D.rotacionaZ(angulo);

    I.Envelope[0] = A + centro;
    I.Envelope[1] = B + centro;
    I.Envelope[2] = C + centro;
    I.Envelope[3] = D + centro;
}

// **********************************************************************
//
// **********************************************************************
void InicializaTemporizador(int personagemIndex) {
    Temporizadores[personagemIndex].tempoRestante = (rand() % 7 + 4);
}

// **********************************************************************
//
// **********************************************************************
void CriaPersonagem(int &i, Ponto posicao, Ponto escala, float rotacao, int idModelo, Ponto pivot, Ponto direcao, float velocidade, bool projetil = false, bool PersonagemPrincipal = false)
{
    Personagens[i].Posicao = posicao;
    Personagens[i].Escala = escala;
    Personagens[i].Rotacao = rotacao;
    Personagens[i].IdDoModelo = idModelo;
    Personagens[i].modelo = DesenhaPersonagemMatricial;
    Personagens[i].Pivot = pivot;
    Personagens[i].Direcao = direcao;
    Personagens[i].Direcao.rotacionaZ(rotacao);
    Personagens[i].Velocidade = velocidade;
    Personagens[i].Ativo = true;
    Personagens[i].Projetil = projetil;
    Personagens[i].PersonagemPrincipal = PersonagemPrincipal;
    Personagens[i + AREA_DE_BACKUP] = Personagens[i];

    if (!PersonagemPrincipal) { 
        InicializaTemporizador(i);
    }

    if (projetil) {
        AtualizaEnvelopeBala(i); 
    } else {
        AtualizaEnvelope(i);
    }

    i++;
}

// **********************************************************************
// Esta função deve instanciar todos os personagens do cenário
// **********************************************************************
void CriaInstancias()
{
    srand(time(0));

    int i = 0;
    CriaPersonagem(i, Ponto(-10, 0), Ponto(1, 1), -45, 0, Ponto(2.5, 0), Ponto(0, 1), 2, false, true); 
    
    float distanciaMinima = 10.0; 

    while (i < nModelos -1) // modificar numero de naves
    {
        Ponto posicao;
        bool posValida = false;
        while (!posValida)
        {
            posicao = Ponto((rand() % 40) - 20, (rand() % 40) - 20); 
            float distancia = sqrt(pow(posicao.x - Personagens[0].Posicao.x, 2) + pow(posicao.y - Personagens[0].Posicao.y, 2));
            if (distancia >= distanciaMinima)
            {
                posValida = true;
            }
        }

        Ponto escala(1, 1);
        float rotacao = rand() % 360;
        Ponto pivot(2.5, 0);
        Ponto direcao(0, 1);
        float velocidade = 2 + rand() % 4;

        bool colisao = false;
        for (int j = 1; j < i; j++)
        {
            Instancia temp = Personagens[j];
            temp.Posicao = posicao;
            temp.Escala = escala;
            temp.Rotacao = rotacao;
            temp.IdDoModelo = 2 + (rand() % (nModelos - 2)); 
            AtualizaEnvelope(j); 
            AtualizaEnvelope(i); 
            if (TestaColisao(j, i))
            {
                colisao = true;
                break;
            }
        }

        if (!colisao)
        {
            CriaPersonagem(i, posicao, escala, rotacao, 2 + (rand() % (nModelos - 2)), pivot, direcao, velocidade);
        }
    }

    nInstancias = i;
}

// **********************************************************************
// Esta função testa a colisao entre os envelopes
// de dois personagens matriciais
// **********************************************************************
bool TestaColisao(int Objeto1, int Objeto2)
{
    // Testa todas as arestas do envelope de
    // um objeto contra as arestas do outro

    for (int i = 0; i < 4; i++)
    {
        Ponto A = Personagens[Objeto1].Envelope[i];
        Ponto B = Personagens[Objeto1].Envelope[(i + 1) % 4];
        for (int j = 0; j < 4; j++)
        {
            Ponto C = Personagens[Objeto2].Envelope[j];
            Ponto D = Personagens[Objeto2].Envelope[(j + 1) % 4];

            if (HaInterseccao(A, B, C, D))
                return true;
        }
    }
    return false;
}

// **********************************************************************
//
// **********************************************************************
void ContagemVidas()
{
    colisoesdisparador = 0;
}

// **********************************************************************
//
// **********************************************************************
void ReiniciarJogo()
{
    ContagemVidas();
    telaCongelada = false;
    std::exit(0);
}

// **********************************************************************
//
// **********************************************************************
void PrintString(string s, int posX, int posY, int cor)
{
    defineCor(cor);
    glRasterPos3i(posX, posY, 0); // define posição na tela
    for (int i = 0; i < s.length(); i++)
    {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s[i]);
    }
}

// **********************************************************************
//
// **********************************************************************
void VerificarColisoesEEstadoDoJogo() 
{
    // colisão entre disparos do personagem principal e inimigos
    for (int i = 1; i < nInstancias; i++) 
    {
        if (Personagens[i].Projetil && Personagens[i].Ativo && Personagens[i].PersonagemPrincipal) 
        { 
            for (int j = 1; j < nInstancias; j++) 
            {
                if (!Personagens[j].Projetil && !Personagens[j].PersonagemPrincipal && Personagens[j].Ativo && TestaColisao(i, j)) 
                { 
                    Personagens[i].Ativo = false;
                    Personagens[j].Ativo = false;
                    balasDispCount--;
                    break;
                }
            }
        }
    }

    // colisão entre disparos dos inimigos e o personagem principal
    for (int i = 1; i < nInstancias; i++) 
    {
        if (Personagens[i].Projetil && Personagens[i].Ativo && !Personagens[i].PersonagemPrincipal) 
        { 
            if (TestaColisao(i, 0)) 
            { 
                Personagens[i].Ativo = false;
                colisoesdisparador++;
                balasDispCount--;
                if (colisoesdisparador >= 3) 
                {
                    telaCongelada = true;
                    mensagemFinal = "GameOver";
                }
                break;
            }
        }
    }

    // colisão entre o personagem principal e inimigos
    for (int i = 1; i < nInstancias; i++) 
    {
        if (!Personagens[i].Projetil && !Personagens[i].PersonagemPrincipal && Personagens[i].Ativo && TestaColisao(0, i)) 
        {
            cout << "Player hit!" << endl;
            colisoesdisparador++;
            Personagens[i].Ativo = false;
            if (colisoesdisparador >= 3) 
            {
                telaCongelada = true;
                mensagemFinal = "GameOver";
            }
        }
    }

    // atualizar vidas
    int remainingLives = 3 - colisoesdisparador;
    string livesText = "Vidas: " + to_string(remainingLives);
    PrintString(livesText, -20, -20, 200);

    // verificar se todas as naves inimigas foram destruídas
    bool todasNavesDestruidas = true;
    for (int i = 1; i < nInstancias; i++) 
    {
        if (!Personagens[i].Projetil && !Personagens[i].PersonagemPrincipal && Personagens[i].Ativo) 
        {
            todasNavesDestruidas = false;
            break;
        }
    }
    if (todasNavesDestruidas) 
    {
        mensagemFinal = "Vitoria!";
        telaCongelada = true;
    }

    // verificar se as vidas do jogador acabaram
    if (colisoesdisparador >= 3) 
    {
        mensagemFinal = "Gameover";
        telaCongelada = true;
    }
}

// **********************************************************************
//
// **********************************************************************
bool TestaColisaoBala(int balasIndex, int personagemIndex)
{
    Ponto A = Personagens[balasIndex].Posicao;
    Instancia &personagem = Personagens[personagemIndex];

    for (int i = 0; i < 4; i++)
    {
        Ponto B = personagem.Envelope[i];
        Ponto C = personagem.Envelope[(i + 1) % 4];

        if (HaInterseccao(A, A + Personagens[balasIndex].Direcao, B, C))
            return true;
    }
    return false;
}

// **********************************************************************
// Esta função calcula o envelope do personagem matricial
// **********************************************************************
void AtualizaEnvelope(int personagem)
{
    if (!Personagens[personagem].Ativo)
        return; 

    Instancia I;
    I = Personagens[personagem];

    ModeloMatricial MM = Modelos[I.IdDoModelo];

    Ponto A;
    Ponto V;
    V = I.Direcao * (MM.nColunas / 2.0);
    V.rotacionaZ(90);
    A = I.PosicaoDoPersonagem + V;

    Ponto B = A + I.Direcao * (MM.nLinhas);

    V = I.Direcao * (MM.nColunas);
    V.rotacionaZ(-90);
    Ponto C = B + V;

    V = -I.Direcao * (MM.nLinhas);
    Ponto D = C + V;

    // desenha o envelope
    defineCor(Red);
    glBegin(GL_LINE_LOOP);
    glVertex2f(A.x, A.y);
    glVertex2f(B.x, B.y);
    glVertex2f(C.x, C.y);
    glVertex2f(D.x, D.y);
    glEnd();

    // armazerna as coordenadas do envelope na instância
    Personagens[personagem].Envelope[0] = A;
    Personagens[personagem].Envelope[1] = B;
    Personagens[personagem].Envelope[2] = C;
    Personagens[personagem].Envelope[3] = D;
}


// **********************************************************************
//
// **********************************************************************
void AtualizaJogo()
{
    // envelopes
    for (int i = 0; i < nInstancias; i++) {
        if (Personagens[i].Ativo) {
            if (Personagens[i].Projetil) {
                AtualizaEnvelopeBala(i);
            } else {
                AtualizaEnvelope(i);
            }
        }
    }

    // atualiza disparos dos inimigos
    AtualizaDisparosInimigos();

    // verifica colisões e estado do jogo
    VerificarColisoesEEstadoDoJogo();

    glutSwapBuffers();
}

// **********************************************************************
//
// **********************************************************************
void AtualizaPersonagens(float tempoDecorrido)
{
    AtualizaDirecaoAleatoria(tempoDecorrido);
    for (int i = 0; i < nInstancias; i++)
    {
        if (Personagens[i].Ativo)
        {
            Personagens[i].AtualizaPosicao(tempoDecorrido);
            LimitaPersonagensTela(Personagens[i]);
        }
    }

    AtualizaJogo();
}

// **********************************************************************
//
// **********************************************************************
void LimitaPersonagensTela(Instancia &personagem)
{
    if (personagem.Posicao.x < Min.x || personagem.Posicao.x > Max.x || personagem.Posicao.y < Min.y || personagem.Posicao.y > Max.y)
    {
        if (personagem.IdDoModelo == 1) // verifica se é uma bala
        {
            personagem.Ativo = false;
            balasDispCount--;
        }
        else
        {
            personagem.Direcao = -personagem.Direcao;
            personagem.Rotacao += 180;

            if (personagem.Posicao.x < Min.x)
            {
                personagem.Posicao.x = Min.x;
            }
            if (personagem.Posicao.x > Max.x)
            {
                personagem.Posicao.x = Max.x;
            }
            if (personagem.Posicao.y < Min.y)
            {
                personagem.Posicao.y = Min.y;
            }
            if (personagem.Posicao.y > Max.y)
            {
                personagem.Posicao.y = Max.y;
            }
        }
    }
}

// **********************************************************************
//
// **********************************************************************
void DesenhaPersonagens()
{
    for (int i = 0; i < nInstancias; i++)
    {
        if (Personagens[i].Ativo)
        {
            PersonagemAtual = i;
            Personagens[i].desenha();
        }
    }
}

// **********************************************************************
//
// **********************************************************************
void DispararDisp(int personagemIndex)
{
    if (balasDispCount < MAX_BALAS_DISP)
    {
        int i = nInstancias;
        Ponto pivot(Modelos[1].nColunas / 2.0, Modelos[1].nLinhas / 2.0);
        CriaPersonagem(i, Personagens[personagemIndex].Posicao, Ponto(1, 1), Personagens[personagemIndex].Rotacao, 1, pivot, Ponto (0,3), 5, true, Personagens[personagemIndex].PersonagemPrincipal);
        Personagens[i - 1].Ativo = true;
        nInstancias++;
        balasDispCount++;
    }
}

// **********************************************************************
//
// **********************************************************************
void AtualizarBalasDisp(double dt)
{
    for (int i = 10; i < nInstancias; ++i) // alterar
    {
        if (Personagens[i].IdDoModelo == 1 && Personagens[i].Ativo)
        {
            Personagens[i].Posicao = Personagens[i].Posicao + Personagens[i].Direcao * Personagens[i].Velocidade * dt;
            AtualizaEnvelopeBala(i);
        }
    }
}

// **********************************************************************
//
// **********************************************************************
void AtualizaDirecaoAleatoria(double deltaT) 
{
    TempoDesdeUltimaAtualizacao += deltaT;
    if (TempoDesdeUltimaAtualizacao < INTERVALO_ATUALIZACAO) 
    {
        return;
    }

    TempoDesdeUltimaAtualizacao = 0.0;

    for (int i = 1; i < nInstancias; i++) 
    {
        if (!Personagens[i].Projetil && !Personagens[i].PersonagemPrincipal) 
        {
            float novoAngulo = rand() % 360;
            Personagens[i].Direcao.rotacionaZ(novoAngulo - Personagens[i].Rotacao);
            Personagens[i].Rotacao = novoAngulo;
            AtualizaEnvelope(i); 
        }
    }
}

// **********************************************************************
//
// **********************************************************************
void DispararInimigo(int personagemIndex)
{
    if (balasDispCount < MAX_BALAS_DISP)
    {
        int i = nInstancias;
        Ponto pivot(Modelos[1].nColunas / 2.0, Modelos[1].nLinhas / 2.0);
        CriaPersonagem(i, Personagens[personagemIndex].Posicao, Ponto(1, 1), Personagens[personagemIndex].Rotacao, 1, pivot, Ponto(0, 3), 5, true, false);
        Personagens[i - 1].Ativo = true;
        nInstancias++;
        balasDispCount++;
    }
}

// **********************************************************************
//
// **********************************************************************
void AtualizaDisparosInimigos()
{
    static auto lastShootTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedTime = currentTime - lastShootTime;

    if (elapsedTime.count() > 0.5) // intervalo de tempo entre os disparos
    {
        for (int i = 1; i < nInstancias; ++i)
        {
            if (!Personagens[i].Projetil && Personagens[i].Ativo)
            {
                if (rand() % 100 < 20) // 20% de chance de disparar
                {
                    DispararInimigo(i);
                }
            }
        }
        lastShootTime = currentTime;
    }
}

// **********************************************************************
//  void display( void )
// **********************************************************************
void display(void)
{
    // Limpa a tela com a cor de fundo
    glClear(GL_COLOR_BUFFER_BIT);

    // Define os limites lógicos da área OpenGL dentro da Janela
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Desenha e atualiza os personagens
    glLineWidth(1);
    glColor3f(1, 1, 1); // R, G, B  [0..1]
    DesenhaEixos();

    if (!telaCongelada)
    {
        DesenhaPersonagens();
        AtualizaPersonagens(T2.getDeltaT());
    }

    if (!mensagemFinal.empty())
    {
        PrintString(mensagemFinal, -5, 0, 200);

        if (tempoInicialMensagem < 0.0) {
            tempoInicialMensagem = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
        } else {
            double tempoAtual = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
            if (tempoAtual - tempoInicialMensagem >= 3.0) {
                exit(0);
            }
        }
    }

    glutSwapBuffers();
}

// **********************************************************************
// ContaTempo(double tempo)
//      conta um certo numero de segundos e informa quanto frames
// se passaram neste período.
// **********************************************************************
void ContaTempo(double tempo)
{
    Temporizador T;

    unsigned long cont = 0;
    cout << "Inicio contagem de " << tempo << " segundos ..." << flush;
    while (true)
    {
        tempo -= T.getDeltaT();
        cont++;
        if (tempo <= 0.0)
        {
            cout << "fim! - Passaram-se " << cont << " frames." << endl;
            break;
        }
    }
}

// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
// **********************************************************************
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27: // Termina o programa qdo
        exit(0); // a tecla ESC for pressionada
        break;
    case 't':
        ContaTempo(3);
        break;
    case ' ': // Tecla de espaço
        DispararDisp(0); // Jogador principal atira
        break;
    case 'd':
        break;
    case 'a':
        break;
    default:
        break;
    }
}

// **********************************************************************
//  void arrow_keys ( int a_keys, int x, int y )
// **********************************************************************
void arrow_keys(int a_keys, int x, int y)
{
    switch (a_keys)
    {
    case GLUT_KEY_LEFT:
        Personagens[0].Rotacao += 5;
        Personagens[0].Direcao.rotacionaZ(5);
        break;
    case GLUT_KEY_RIGHT:
        Personagens[0].Rotacao -= 5;
        Personagens[0].Direcao.rotacionaZ(-5);
        break;
    case GLUT_KEY_UP: // Se pressionar UP
        Personagens[0].Velocidade++;
        break;
    case GLUT_KEY_DOWN: // Se pressionar UP
        Personagens[0].Velocidade--;
        break;
    default:
        break;
    }
}

// **********************************************************************
//  void main ( int argc, char** argv )
//
// **********************************************************************
int main(int argc, char **argv)
{
    cout << "Programa OpenGL" << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    glutInitWindowPosition(0, 0);

    // Define o tamanho inicial da janela gráfica do programa
    glutInitWindowSize(400, 400);

    // Cria a janela na tela, definindo o nome da
    // que aparecerá na barra de título da janela.
    glutCreateWindow("AsteroideGame");

    // executa algumas inicializações
    init();

    // Define que o tratador de evento para
    // o redesenho da tela. A função "display"
    // será chamada automaticamente quando
    // for necessário redesenhar a janela
    glutDisplayFunc(display);

    // Define que o tratador de evento para
    // o invalidação da tela. A função "display"
    // será chamada automaticamente sempre que a
    // máquina estiver ociosa (idle)
    glutIdleFunc(animate);

    // Define que o tratador de evento para
    // o redimensionamento da janela. A função "reshape"
    // será chamada automaticamente quando
    // o usuário alterar o tamanho da janela
    glutReshapeFunc(reshape);

    // Define que o tratador de evento para
    // as teclas. A função "keyboard"
    // será chamada automaticamente sempre
    // o usuário pressionar uma tecla comum
    glutKeyboardFunc(keyboard);

    // Define que o tratador de evento para
    // as teclas especiais(F1, F2,... ALT-A,
    // ALT-B, Teclas de Seta, ...).
    // A função "arrow_keys" será chamada
    // automaticamente sempre o usuário
    // pressionar uma tecla especial
    glutSpecialFunc(arrow_keys);

    // inicia o tratamento dos eventos
    glutMainLoop();

    return 0;
}