#ifndef GROUP_USER
#define GROUP_USER

#include "user.hpp"

class GroupUser : public User
{  
public:
    void setRole(string role){this->role = role;}
    string getRole(){return this->role;}
private:
    string role;

};



#endif
