#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#include "rc522.h"
#include "ds1307.h"
#include <stdint.h>

#define MAX_AUTHORIZED_CARDS  10

typedef enum {
    ACCESS_GRANTED = 0,
    ACCESS_DENIED
} AccessResult;

/* Add your authorized card UIDs here */
extern const uint8_t authorizedCards[MAX_AUTHORIZED_CARDS][4];
extern uint8_t       numAuthorizedCards;

AccessResult  AccessControl_CheckUID(const RC522_UID *uid);
void          AccessControl_LogEvent(const RC522_UID *uid, AccessResult result, const DS1307_Time *t);
void          AccessControl_GrantAccess(void);
void          AccessControl_DenyAccess(void);
void          AccessControl_MotionAlert(const DS1307_Time *t);

#endif /* ACCESS_CONTROL_H */
