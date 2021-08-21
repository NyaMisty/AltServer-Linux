#ifdef __cplusplus
extern "C" {
#endif
    int initGlobalDevice();
    int initHeartbeat(void **hbclient);
    int do_heartbeat(void *_client);
    int initUPnP();
    void setupPairInfo(const char *udid, const char *ipaddr, const char *pairDataFile);
#ifdef __cplusplus
}
#endif