namespace xkv {

class cli {

  private:
    // 开始接收输入
    void start_input();

  public:
    // 开始循环并处理异常
    void loop();
    // 新建进程并运行cli循环
    static bool create_process_if_necessary(int argc, char **argv);
};

}
