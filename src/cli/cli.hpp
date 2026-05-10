namespace xkv {

class cli {

  private:
    // 开始接收输入
    void start_input();
    // 开始循环并处理异常
    void loop();

  public:
    // 新建进程并运行cli循环
    void run();
};

}
