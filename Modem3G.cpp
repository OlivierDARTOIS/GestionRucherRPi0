#include "Modem3G.h"
//#include "serialib.h"

Modem3G::Modem3G(std::string portSerie, int vitesse) {
   this->nomPortSerie = portSerie;
   this->vitesse = vitesse;
   this->erreur.clear();
}

bool Modem3G::connecter()
{
    
   int codeRetour = VS3G.Open(nomPortSerie.c_str(), this->vitesse);
   if (codeRetour == 1) {
       VS3G.FlushReceiver();
       return true;
   }
   else {
       this->erreur = "Ouverture impossible voie serie";
       return false;
   }
}

bool Modem3G::estPret()
{
   std::string cmd = cmdAT + caracCR;
   
   int codeRetour;

    VS3G.FlushReceiver();
    char reponse[100];
    int cpt = 0;
    do {
        codeRetour = VS3G.WriteString(cmd.c_str());
        if (codeRetour < 0) {
            this->erreur = "Erreur ecriture sur voie serie methode estPret() code:"
                    + std::to_string(codeRetour);
            return false;
        }
        codeRetour = VS3G.ReadString(reponse, 0x0A, 100, 1000); //0x0A = LF
        if (codeRetour < 1) {
            cpt++;
        }
        else
            break;
    } while (cpt < 2);
    
   if (cpt == 2) {
       this->erreur = "Erreur lecture premiere ligne methode estPret() code:" 
               + std::to_string(codeRetour);
       return false;
   }
   
   codeRetour = VS3G.ReadString(reponse, 0x0A, 100, 1500);
   if (codeRetour < 1) {
       this->erreur = "Erreur lecture deuxieme ligne methode estPret() code:" 
               + std::to_string(codeRetour);
       return false;
   }
   std::string rep(reponse);
   
   //std::cout << "cmdAT : " << rep << std::endl;
   //std::cout << "cmdAT : " << rep2 << std::endl;
   if ( rep.substr(0,2) == "OK" )
	return true;
   else
	return false;
}

bool Modem3G::estEnregistreSurReseau()
{
   std::string cmd = cmdEnrSurReseau + caracCR;
   
   int codeRetour = VS3G.WriteString(cmd.c_str());
   if (codeRetour < 0) {
       this->erreur = "Erreur ecriture methode estEnrengistreSurReseau() code:" 
               + std::to_string(codeRetour);
	return false;
   }
   
   VS3G.FlushReceiver();
   char reponse[100];
   codeRetour = VS3G.ReadString(reponse, 0x0A, 100, 500);
   if (codeRetour < 1) {
        this->erreur = "Erreur lecture premiere ligne methode estEnrengistreSurReseau() code:" 
               + std::to_string(codeRetour);
	return false;
   }
   
   codeRetour = VS3G.ReadString(reponse, 0x0A, 100, 500);
   if (codeRetour < 1) {
       this->erreur = "Erreur lecture deuxieme ligne methode estEnrengistreSurReseau() code:" 
               + std::to_string(codeRetour);
	return false;
   }
   
   std::string rep(reponse);
   rep.pop_back();  // suppression CR + LF de la réponse
   rep.pop_back();
   
   if (rep == enrReseauNatif)   return true;
   if (rep == enrReseauRoaming) return true;
   
   return false;
}

int Modem3G::getNivRecep()
{
   char temp;
   while (VS3G.ReadChar(&temp, 100)>0);
   
   VS3G.FlushReceiver();

   std::string cmd = cmdNivRecep + caracCR;
   
   int codeRetour = VS3G.WriteString(cmd.c_str());
   if (codeRetour < 0) {
        this->erreur = "Erreur ecriture methode getNivRecep() code:" 
               + std::to_string(codeRetour);
	return -1;
   }
   
   char reponse[100] = {0};
   codeRetour = VS3G.ReadString(reponse, 0x0A, 100, 1500);
   if (codeRetour < 1) {
       this->erreur = "Erreur lecture premiere ligne methode getNivRecep() code:" 
               + std::to_string(codeRetour);
	return -1;
   }
   
   codeRetour = VS3G.ReadString(reponse, 0x0A, 100, 500);
   if (codeRetour < 1) {
       this->erreur = "Erreur lecture deuxieme ligne methode getNivRecep() code:" 
               + std::to_string(codeRetour);
	return -1;
   }
   
   std::string rep(reponse); // la deuxieme : +CSQ: XX,0
   
   //std::cout << "getNivRecep : " << rep << std::endl;
   //std::cout << "getNivRecep : " << rep2 << std::endl;
   
   VS3G.FlushReceiver();
   
   return std::stoi(rep.substr(6,2));
}

void Modem3G::deconnecter()
{
   VS3G.Close();
}

std::string Modem3G::getErreur() 
{
    std::string temp = erreur;
    erreur.clear();
    return temp;
}