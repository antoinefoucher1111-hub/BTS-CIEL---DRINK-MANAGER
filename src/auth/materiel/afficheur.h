#include <string>


class IAfficheur {
public:
    virtual ~IAfficheur() = default;
    virtual void afficher(const std::string& texte) = 0;
};