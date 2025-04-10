class FdCtx{
    public:
    FdCtx(int fd);
    ~FdCtx();
    bool init();
    bool isInit(){return m_isInit;};
    bool isSocket(){return m_isSocket;};
    bool isClose(){return m_isClose;};

    void setUserNonblock(bool v){m_userNonblock=v;};
    bool getUserNonblock(){return m_userNonblock;};

    void setSysNonblock(bool v){m_sysNonblock=v;};
    bool getSysNonblock(){return m_sysNonblock;};
    private:
    int m_fd;
    bool m_isInit;
    bool m_isSocket;
    bool m_isClose;
    bool m_userNonblock;
    bool m_sysNonblock;
};