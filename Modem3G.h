#ifndef _MODEM3G_H_
#define _MODEM3G_H_

#include <string>
#include "serialib.h"

class Modem3G
{
   private:
	serialib VS3G;
	std::string nomPortSerie;
        int vitesse;
        std::string erreur;
	const std::string cmdAT = "AT";
        const std::string cmdMarqueModem = "ATI";
        const std::string marqueModem = "SIM800";
        const std::string cmdEnrSurReseau = "AT+CREG?";
        const std::string enrReseauNatif = "+CREG: 0,1";
        const std::string enrReseauRoaming = "+CREG: 0,5";
        const std::string enrReseauEnCours = "+CREG: 0,2";
	const std::string cmdNivRecep = "AT+CSQ";
	const std::string caracCR = "\r";
	const std::string caracLF = "\n";

   public:
	Modem3G(std::string portSerie, int vitesse);
	bool connecter();
	void deconnecter();
	bool estPret();
        bool estEnregistreSurReseau();
	int  getNivRecep();
        std::string getErreur();
        
        const int nivMiniRecep = 10; 
};

#endif

