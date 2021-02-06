int StartSeconds = 0;
int SysMilliseconds()
{
    struct timeval tp;
    int CTime;
    gettimeofday(&tp, NULL);
    if ( !StartSeconds ){
        StartSeconds = tp.tv_sec;
        return tp.tv_usec/1000;
    }
    CTime = (tp.tv_sec - StartSeconds)*1000 + tp.tv_usec / 1000;
    return CTime;
} 
