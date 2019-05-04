/* 
 * File:   main.cpp
 * Author: Olivier
 *
 * Created on 22 mars 2019, 16:24
 */

#include <iostream>
#include <string>
#include <algorithm>    // Pour trier un vector par ordre croissant (médiane)
#include <random>       // tirage d'un nombre au hasard
#include <vector>
#include <fstream>
#include <ctime>        // récupération date/heure
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include <stdlib.h>     // pour l'appel system()
#include <unistd.h>     // pour sleep()

#include "Modem3G.h"
#include "Modem3G.h"     // Classe de gestion du modem3G

class Ruche {
private:
    std::string nom;
    int numeroBusI2c;
    bool captTempHumOK;
    bool captMasseOK;
    float temperature;
    float humidite;
    float pression;
    float masse;
    std::string busI2C;
    std::string iioDeviceCaptBME280;
    std::string iioDeviceCaptMCP3421;
    std::string erreur;

    // Adresses par défaut des capteurs sur le bus I2C
    const int addrBME280 = 0x76;
    const int addrMCP3421 = 0x6c;

    // Fichiers pour les donnees du capteur BME280
    const std::string fichTemp = "in_temp_input";
    const std::string fichHum = "in_humidityrelative_input";
    const std::string fichPression = "in_pressure_input";

    // Fichiers pour configurer et lire une tension issue du CAN MCP3421/3424
    const std::string fichVoltBrute = "in_voltage0_raw";
    const std::string fichFreqEchantillonage = "in_voltage_sampling_frequency";
    const std::string fichVoltEchelle = "in_voltage0_scale";

public:
    Ruche(std::string nom, int numero);
    ~Ruche();
    bool initialiserCapteurs();
    bool lireCapteurs();
    bool getCaptTempHumOK() const;
    bool getCaptMasseOK() const;
    std::string getNom() const;
    float getTemperature() const;
    float getHumidite() const;
    float getPression() const;
    float getMasse() const;
    std::string getErreur();

    static int numIIODevice;
    static int nbRuchesMax;

    static void detectionNbRuchesMax();
};

Ruche::Ruche(std::string nom, int numero) {
    if (nom.size() != 0)
        this->nom = nom;
    else
        this->nom = "Inconnu";

    this->numeroBusI2c = numero;
    this->busI2C = "/dev/i2c-" + std::to_string(numero);

    this->captMasseOK = false;
    this->captTempHumOK = false;
    this->temperature = 0;
    this->humidite = 0;
    this->pression = 0;
    this->masse = 0;
    this->erreur = "Aucune erreur";
}

Ruche::~Ruche() {
    // Suppression du capteur BME280
    if (this->captTempHumOK) {
        std::string commande = "echo \"" + std::to_string(this->addrBME280) +
                "\" > /sys/class/i2c-dev/i2c-" +
                std::to_string(this->numeroBusI2c) + "/device/delete_device";
        int codeRetour = system(commande.c_str());
        if (codeRetour == 0) {
            Ruche::numIIODevice--;
            this->captTempHumOK = false;
        }
    }

    // Suppression du capteur MCP3421
    if (this->captMasseOK) {
        std::string commande = "echo \"" + std::to_string(this->addrMCP3421) +
                "\" > /sys/class/i2c-dev/i2c-" +
                std::to_string(this->numeroBusI2c) + "/device/delete_device";
        int codeRetour = system(commande.c_str());
        if (codeRetour == 0) {
            Ruche::numIIODevice--;
            this->captMasseOK = false;
        }
    }
}

bool Ruche::getCaptTempHumOK() const {
    return this->captTempHumOK;
}

bool Ruche::getCaptMasseOK() const {
    return this->captMasseOK;
}

float Ruche::getTemperature() const {
    return this->temperature;
}

float Ruche::getHumidite() const {
    return this->humidite;
}

float Ruche::getPression() const {
    return this->pression;
}

float Ruche::getMasse() const {
    return this->masse;
}

std::string Ruche::getNom() const {
    return this->nom;
}

bool Ruche::initialiserCapteurs() {
    bool initCapteur = true;

    // test pour voir si bus i2c présent pour capteur BME280
    int fd = wiringPiI2CSetupInterface(this->busI2C.c_str(), this->addrBME280);
    if (fd < 0) {
        this->erreur = "Ouverture bus i2c impossible";
        return false;
    }

    // test pour voir si capteur BME280 présent
    int res = wiringPiI2CRead(fd);
    if (res < 0) {
        this->erreur = "Capteur BME280 introuvable";
        initCapteur = false;
    } else {
        std::string commande = "echo \"bme280 " +
                std::to_string(this->addrBME280) +
                "\" > /sys/bus/i2c/devices/i2c-" +
                std::to_string(this->numeroBusI2c) + "/new_device";
        int codeRetour = system(commande.c_str());

        if (codeRetour != 0) {
            this->erreur = "Creation capteur BME280 impossible";
            initCapteur = false;
        } else {
            this->iioDeviceCaptBME280 = "/sys/bus/iio/devices/iio:device" +
                    std::to_string(Ruche::numIIODevice) + "/";
            Ruche::numIIODevice++;
            sleep(1); // Temps de creation du capteur par le noyau
            this->captTempHumOK = true;
        }
    }
    close(fd);
    sleep(1);
    // test pour voir si bus i2c présent pour capteur MCP3421
    fd = wiringPiI2CSetupInterface(this->busI2C.c_str(), this->addrMCP3421);
    if (fd < 0) {
        this->erreur = "Ouverture bus i2c impossible";
        return false;
    }

    // test pour voir si capteur MCP3421 présent
    res = wiringPiI2CRead(fd);
    if (res < 0) {
        this->erreur += "Capteur MCP3421/3424 introuvable";
        initCapteur = false;
    } else {
        std::string commande = "echo \"mcp3424 " +
                std::to_string(this->addrMCP3421) +
                "\" > /sys/bus/i2c/devices/i2c-" +
                std::to_string(this->numeroBusI2c) + "/new_device";
        int codeRetour = system(commande.c_str());

        if (codeRetour != 0) {
            this->erreur += "Creation capteur MCP3421/3424 impossible";
            initCapteur = false;
        } else {
            this->iioDeviceCaptMCP3421 = "/sys/bus/iio/devices/iio:device" +
                    std::to_string(Ruche::numIIODevice) + "/";
            Ruche::numIIODevice++;
            sleep(1); // Temps de creation du capteur par le noyau

            // Mettre le CAN MCP3421 en mode 18 bits
            std::string nomFich = this->iioDeviceCaptMCP3421 +
                    this->fichFreqEchantillonage;
            std::ofstream mcp3421(nomFich);
            if (mcp3421.is_open()) {
                mcp3421 << "3"; // '3' echantillon par seconde -> res. de 18bits
                mcp3421.close();
            }
            // Mettre le PGA interne du CAN MCP3421 en x8
            nomFich = this->iioDeviceCaptMCP3421 + this->fichVoltEchelle;
            mcp3421.open(nomFich);
            if (mcp3421.is_open()) {
                mcp3421 << "0.000001953";
                mcp3421.close();
            }
            this->captMasseOK = true;
        }
    }

    return initCapteur;
}

bool Ruche::lireCapteurs() {
    bool lectCapteur = true;

    if (this->captTempHumOK) {
        std::string nomFich = this->iioDeviceCaptBME280 + this->fichTemp;
        std::ifstream bme280(nomFich);

        if (bme280.is_open()) {
            int temp = 0;
            bme280 >> temp;
            bme280.close();
            this->temperature = temp / 1000.0;
        } else {
            this->erreur = "Probleme lecture temperature BME280";
            lectCapteur = false;
        }

        nomFich = this->iioDeviceCaptBME280 + this->fichHum;
        bme280.open(nomFich);

        if (bme280.is_open()) {
            int hum = 0;
            bme280 >> hum;
            bme280.close();
            this->humidite = hum / 1000.0;
        } else {
            this->erreur += "\nProbleme lecture humidite BME280";
            lectCapteur = false;
        }

        nomFich = this->iioDeviceCaptBME280 + this->fichPression;
        bme280.open(nomFich);

        if (bme280.is_open()) {
            bme280 >> this->pression;
            bme280.close();
            this->pression = this->pression * 10.0;
        } else {
            this->erreur += "\nProbleme lecture pression BME280";
            lectCapteur = false;
        }
    }

    if (this->captMasseOK) {
        std::string nomFich = this->iioDeviceCaptMCP3421 + this->fichVoltBrute;
        std::ifstream mcp3421(nomFich);
        std::vector<int> mesures; // vecteur pour stocker les conversions

        if (mcp3421.is_open()) {
            int temp = 0;
            for (int i = 0; i < 11; i++) { // 11 mesures
                mcp3421 >> temp;
                mesures.push_back(temp);
                sleep(1); // rappel: 3 échantillons/s donc on va prendre tts les secondes
            }
            mcp3421.close();

            // Filtrage par la médiane
            std::sort(mesures.begin(), mesures.end()); // on trie par ordre croissant

            this->masse = mesures.at(5) * 33.4 - 25620.0; // on prend l'échantillon médian
            this->masse = this->masse / 1000.0;
        } else {
            this->erreur += "\nProbleme lecture masse MCP3421/3424";
            lectCapteur = false;
        }
    }

    return lectCapteur;
}

std::string Ruche::getErreur() {
    std::string temp = this->erreur;
    this->erreur = "Aucune erreur";
    return temp;
}

void Ruche::detectionNbRuchesMax() {
    const int addrPCA9548Carte1 = 0x70;
    const int nbMaxCartes = 4;

    for (int i = 0; i < nbMaxCartes; i++) {
        int fd = wiringPiI2CSetupInterface("/dev/i2c-1", addrPCA9548Carte1 + i);
        if (fd < 0) {
            std::cout << "Ouverture bus i2c impossible" << std::endl;
            exit(0);
        }
        int res = wiringPiI2CRead(fd);
        if (res < 0) {
            std::cout << "Adresse 0x" << std::hex << addrPCA9548Carte1 + i <<
                    " : Pas de carte additionnelle détectée" << std::dec <<
                    std::endl;
        } else {
            std::cout << "Adresse 0x" << std::hex << addrPCA9548Carte1 + i <<
                    " : Carte détectée" << std::dec << std::endl;
            std::cout << "Creation bus i2c..." << std::endl;
            std::string commande = "dtoverlay i2c-mux pca9548 addr=" +
                    std::to_string(addrPCA9548Carte1 + i);
            system(commande.c_str());
            sleep(2);
            std::cout << "Création OK" << std::endl;
            Ruche::nbRuchesMax += 8;
        }
    }
}

int Ruche::numIIODevice = 0;
int Ruche::nbRuchesMax = 0;

/*
 * Classe pour envoi des SMS (utilisation de gammu)
 */
class SMS {
private:
    std::string numTel;
    std::string message;
    std::string erreur;
public:
    SMS();
    SMS(std::string numTel, std::string message);
    void setNumTel(std::string numTel);
    void setMessage(std::string message);
    bool envoyer();
    std::string getErreur();
};

SMS::SMS() {
    this->numTel.clear();
    this->message.clear();
    this->erreur = "Aucune erreur";
}

SMS::SMS(std::string numTel, std::string message) {
    if (numTel.size() == 10)
        this->numTel = numTel;
    else
        this->numTel.clear();

    this->message = message;
    this->erreur = "Aucune erreur";
}

void SMS::setNumTel(std::string numTel) {
    if (numTel.size() == 10)
        this->numTel = numTel;
    else
        this->numTel.clear();
}

void SMS::setMessage(std::string message) {
    this->message = message;
}

bool SMS::envoyer() {
    if (this->numTel.size() == 0) {
        this->erreur = "Numero de telephone incorrect";
        return false;
    }

    if (this->message.size() == 0) {
        this->erreur = "Message incorrect";
        return false;
    }

    std::string commandeGammu = "gammu sendsms EMS " + this->numTel +
            " -text " + message;

    int codeRetour = system(commandeGammu.c_str());

    if (codeRetour != 0) {
        this->erreur = "Erreur envoi SMS";
        return false;
    }

    return true;
}

std::string SMS::getErreur() {
    std::string temp = this->erreur;
    this->erreur = "Aucune erreur";
    return temp;
}

/*
 * Classe gestion WittyPi2
 */
class WittyPi {
private:
    std::string cheminScheduleWpi;
    int nbMinutesEteint;
    std::string erreur;
public:
    WittyPi(std::string cheminSchedule);
    bool setNbMinutesEteint(int nbMinutes);
    std::string getErreur();
};

WittyPi::WittyPi(std::string cheminSchedule = "/home/pi/wittyPi/schedule.wpi") {
    this->cheminScheduleWpi = cheminSchedule;
    this->erreur = "";
}

bool WittyPi::setNbMinutesEteint(int nbMinutes) {
    std::ofstream fichSchedule(this->cheminScheduleWpi);
    if (!fichSchedule.is_open()) {
        this->erreur = "Ouverture fichier schedule impossible";
        return false;
    }

    fichSchedule << "BEGIN 2019-01-01 00:00:00" << std::endl;
    fichSchedule << "END   2030-12-31 23:59:59" << std::endl;
    fichSchedule << "ON    M1 WAIT" << std::endl;

    int nbHeure = 0;
    while (nbMinutes > 59) {
        nbHeure++;
        nbMinutes -= 60;
    }
    if (nbHeure > 0)
        fichSchedule << "OFF   H" << std::to_string(nbHeure) <<
        " M" << std::to_string(nbMinutes) << std::endl;
    else
        fichSchedule << "OFF   M" << std::to_string(nbMinutes) << std::endl;

    fichSchedule.close();
    return true;
}

std::string WittyPi::getErreur() {
    std::string temp = this->erreur;
    this->erreur.clear();
    return temp;
}

/*
 * Classe configRucher : parametre de configuration 
 * Fichier de conf dans /boot car la partition est en FAT32 donc accessible
 * depuis windows, le nom du fichier de conf sera : rucher.conf
 * Pour l'instant il doit respecter le formalisme suivant:
 * 1 ere ligne : nom du rucher, sans accent, sans espace
 * 2 eme ligne : numero de telephone du collecteur : 06 ou 07...
 * 3 eme ligne : à déterminer
 * les lignes qui commencent par le caractere # sont ignorées
 */
class ConfigRucher {
private:
    std::string nomFichConf;
    std::string numTel;
    std::string nomRucher;
    std::string erreur;

public:
    ConfigRucher(std::string nomFichConf);
    bool lireFichier();
    std::string getNomRucher() const;
    std::string getNumTel() const;
    std::string getErreur();
};

ConfigRucher::ConfigRucher(std::string nomFichConf = "/boot/rucher.conf") {
    this->nomFichConf = nomFichConf;
    this->numTel.clear();
    this->nomRucher.clear();
    this->erreur.clear();
}

bool ConfigRucher::lireFichier() {
    std::ifstream fichConf(this->nomFichConf);
    if (!fichConf.is_open()) {
        this->erreur = "Impossible de lire le fichier de configuration : " +
                this->nomFichConf;
        return false;
    }

    fichConf >> this->nomRucher;
    fichConf >> this->numTel;

    fichConf.close();
    return true;
}

std::string ConfigRucher::getNomRucher() const {
    return this->nomRucher;
}

std::string ConfigRucher::getNumTel() const {
    return this->numTel;
}

std::string ConfigRucher::getErreur() {
    std::string temp = this->erreur;
    this->erreur.clear();
    return temp;
}

/*
 * Programme principal
 */
int main(int argc, char** argv) {

    // Lecture et analyse du fichier la de configuration du rucher
    // Par défaut : /boot/rucher.conf
    ConfigRucher monRucher;
    if (!monRucher.lireFichier()) {
        std::cout << monRucher.getErreur() << std::endl;
        return 0;
    }

    std::string numTel = monRucher.getNumTel();
    std::string messageSMS = "\"" + monRucher.getNomRucher() + "\n";

    // Envoi de l'heure locale dans le sms
    std::time_t result = std::time(NULL);
    char mbstr[30];
    std::strftime(mbstr, sizeof (mbstr), "%d/%m/%y %H:%M:%S", std::localtime(&result));
    std::string dateheure(mbstr);
    messageSMS += dateheure + ";\n";

    // Appel de la méthode statique pour déterminer le nombre maximum de ruches
    // connectables (forcement un multiple de 8)
    Ruche::detectionNbRuchesMax();
    std::cout << "Nb ruches max : " << Ruche::nbRuchesMax << std::endl;

    // Lecture de toutes les ruches connectables
    // on commence à 3 car c'est le premier bus i2c créé par les PCA
    for (int i = 3; i < Ruche::nbRuchesMax + 3; i++) {
        std::string nomRuche = "Ruche" + std::to_string(i - 2);
        Ruche r(nomRuche, i);

        std::cout << "\nRuche : " << r.getNom() << std::endl;
        messageSMS += r.getNom() + ";";

        if (!r.initialiserCapteurs())
            std::cout << r.getErreur() << std::endl;

        if (!r.lireCapteurs())
            std::cout << r.getErreur() << std::endl;

        if (r.getCaptTempHumOK()) {
            std::cout << "Temp BME280 : " << r.getTemperature() << "°C" << std::endl;
            messageSMS += std::to_string(r.getTemperature()) + ";";
            std::cout << "Hum  BME280 : " << r.getHumidite() << "%HR" << std::endl;
            messageSMS += std::to_string(r.getHumidite()) + ";";
            std::cout << "Pres BME280 : " << r.getPression() << "hPa" << std::endl;
            messageSMS += std::to_string(r.getPression()) + ";";
        } else {
            std::cout << "Pas de capteur BME280" << std::endl;
            messageSMS += "0;0;0;";
        }

        if (r.getCaptMasseOK()) {
            std::cout << "Masse : " << r.getMasse() << "Kg" << std::endl;
            messageSMS += std::to_string(r.getMasse()) + ";\n";
        } else {
            std::cout << "Pas de capteur de masse MCP3421/3424" << std::endl;
            messageSMS += "0;\n";
        }
    }
    messageSMS += "\"";

    // Debug
    std::cout << messageSMS << std::endl;

    // Detection Modem, pret à emttre SMS
    // Attention la vitesse de la voie serie doit être la même que celle
    // configurée dans le fichier /etc/gammurc : ici 19200
    Modem3G sim800c("/dev/ttyAMA0", 19200);
    if (!sim800c.connecter()) {
        std::cout << sim800c.getErreur() << std::endl;
        return 0;
    }
    if (!sim800c.estPret()) {
        std::cout << sim800c.getErreur() << std::endl;
        sim800c.deconnecter();
        return 0;
    }
    if (!sim800c.estEnregistreSurReseau()) {
        std::cout << sim800c.getErreur() << std::endl;
        sim800c.deconnecter();
        return 0;
    }
    int nivRecep = 0;
    if ((nivRecep = sim800c.getNivRecep()) < sim800c.nivMiniRecep) {
        if (nivRecep < 0) {
            std::cout << sim800c.getErreur() << std::endl;
            sim800c.deconnecter();
            return 0;
        } else
            std::cout << "Niveau reception faible" << std::endl;
    } else {
        std::cout << "Niveau reception : " << nivRecep << std::endl;
    }
    sim800c.deconnecter();
    
    // Envoie du SMS
    SMS s(numTel, messageSMS);
    if (!s.envoyer()) {
        std::cout << s.getErreur() << std::endl;
    }

    // Tirage d'un nombre aléatoire entre 5 et 90 (durée arrêt rpi0 en minutes)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> tirage(2, 5);
    int nbMinutesEteint = tirage(gen);

    // Création du fichier de configuration pour la wittyPi2
    WittyPi witty;
    if (!witty.setNbMinutesEteint(nbMinutesEteint)) {
        std::cout << "Erreur wittyPi : " << witty.getErreur() << std::endl;
    } else {
        std::cout << "WittiPy : redemarrage dans " << nbMinutesEteint <<
                " minutes" << std::endl;
    }

    // Je préfère mettre l'extinction de la Pi dans un script
    //sleep(10);
    //system("poweroff");

    return 0;
}

