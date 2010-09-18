
#include "scard.h"

int init_terminal(
        SCARDCONTEXT *hContext,
        SCARDHANDLE *hCard)
{
    LONG rv;
    LPSTR mszReaders;
    DWORD dwReaders;
    DWORD dwAtrLen,dwProtocol,dwState,dwReaderLen;
    BYTE pbAtr[MAX_ATR_SIZE];
    
    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, hContext);
    rv = SCardListReaders(*hContext, NULL, NULL, &dwReaders);
    mszReaders = malloc(sizeof(char)*dwReaders);
    rv = SCardListReaders(*hContext, NULL, mszReaders, &dwReaders);
    if(rv)
    {
        printf("ListReader error: %s\n",pcsc_stringify_error(rv));
        free(mszReaders);
        return rv;
    }
    printf("Reader=%s\n",mszReaders);
    
    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, hContext);
    if(rv)
    {
        printf("Connect to reader error: %s\n",pcsc_stringify_error(rv));
        free(mszReaders);
        return rv;
    }
    
    rv = SCardConnect(*hContext, mszReaders, SCARD_SHARE_SHARED,
                       SCARD_PROTOCOL_T0, hCard, &dwProtocol);
    if(rv == SCARD_E_NO_SMARTCARD //pcsc preference !=
        || rv == SCARD_W_REMOVED_CARD //winscard preference
        )
    {
        printf("Please insert a card in terminal \"%s\"\n",mszReaders);
    }
    
    while(rv == SCARD_E_NO_SMARTCARD  
        || rv == SCARD_W_REMOVED_CARD)
    {
        //wait for a card to be inserted
        usleep(100);
        rv = SCardConnect(*hContext, mszReaders, SCARD_SHARE_SHARED,
                           SCARD_PROTOCOL_T0, hCard, &dwProtocol);
    }

    
    if(rv)
    {
        printf("Connect to card error: %s : err=%X\n",pcsc_stringify_error(rv),rv);
        free(mszReaders);
        return rv;
    }
    
    dwAtrLen = sizeof(pbAtr);
    rv=SCardStatus(*hCard, NULL, &dwReaderLen, &dwState, &dwProtocol,
                   pbAtr, &dwAtrLen);
    
    if(rv)
    {
        printf("Reading ATR error: %s\n",pcsc_stringify_error(rv));
        free(mszReaders);
        return rv;
    }
    else
    {
        int i;
        printf("Reading ATR: ");
        for(i=0;i<dwAtrLen;i++)
            printf("%.2X ",pbAtr[i]);
        printf("\n");
    }
    
    //TODO : have a better control 
//     if(!(pbAtr[5]=='S' && pbAtr[6]=='M'))
//     {
//         printf("This card isn't a smews Card\n");
//         rv = 1;
//     }
       
    
    free(mszReaders);
    return rv;
}


void release_terminal(
        SCARDCONTEXT *hContext,
        SCARDHANDLE *hCard)
{
    LONG rv;
    rv = SCardDisconnect(*hCard, SCARD_UNPOWER_CARD);
    if(rv)printf("Disconnect=%s\n",pcsc_stringify_error(rv));
    rv = SCardReleaseContext(*hContext);
    if(rv)printf("Release=%s\n",pcsc_stringify_error(rv));
    printf("...Exiting \n");
}

int check_terminal_status(SCARDCONTEXT hContext,SCARDHANDLE hCard)
{
    
    LONG rv;
    DWORD dwAtrLen,dwProtocol,dwState,dwReaderLen;
    BYTE pbAtr[MAX_ATR_SIZE];
    int c= 0;
    
    dwAtrLen = sizeof(pbAtr);
                    
    rv=SCardStatus(hCard, NULL, &dwReaderLen, &dwState, &dwProtocol,
                   pbAtr, &dwAtrLen);
                    
    if(dwState)
        printf("State= %X, Possible diagnostic :\n",dwState);  
    
    
    if((dwState & SCARD_ABSENT) == SCARD_ABSENT	)
        printf("There is no card in the reader.\n");
    if((dwState & SCARD_PRESENT)  == SCARD_PRESENT) 	
        printf("There is a card in the reader, but it has not been moved into position for use.\n");
    if((dwState & SCARD_SWALLOWED) == 	SCARD_SWALLOWED)
        printf("There is a card in the reader in position for use. The card is not powered.\n");
    if((dwState & SCARD_POWERED)  == SCARD_POWERED)	
        printf("Power is being provided to the card, but the reader driver is unaware of the mode of the card.\n");
    if((dwState & SCARD_NEGOTIABLE) == SCARD_NEGOTIABLE )	
        printf("The card has been reset and is awaiting PTS negotiation.\n");
    if((dwState & SCARD_SPECIFIC) == SCARD_SPECIFIC)	
        printf("The card has been reset and specific communication protocols have been established.\n");
    
    
    rv= 1;
    while(rv != SCARD_S_SUCCESS)
    {
        if(c==RECONNECTION_COUNT)
            return -1;
        c++;
        printf("Trying to reconnect in %ds...\n",RECONNECTION_DELAY);
        sleep(RECONNECTION_DELAY);
        printf("Trying to reconnect... %d/%d\n",c,RECONNECTION_COUNT);
        rv= SCardReconnect(hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0,
                           SCARD_RESET_CARD, &dwProtocol);
        printf("%s\n",pcsc_stringify_error(rv));
    }
    
    return 0;
}

