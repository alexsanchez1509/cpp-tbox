#ifndef TBOX_NETWORK_BUFFER_H_20171028
#define TBOX_NETWORK_BUFFER_H_20171028

#include <stdint.h>

namespace tbox {
namespace network {

/**
 * ��������
 *
 * buffer_ptr_                        buffer_size_
 *   |                                      |
 *   v                                      V
 *   +----+----------------+----------------+
 *   |    | readable bytes | writable bytes |
 *   +----+----------------+----------------+
 *        ^                ^
 *        |                |
 *    read_index_       write_index_
 *
 * ʹ��ʾ����
 *  Buffer b;
 *  b.append("abc", 4);  //! ��������д��4�ֽڵ�����
 *  char buff[10];
 *  b.fetch(buff, 4);    //! �ӻ�����ȡ��4�ֽڵ�����
 *
 *  b.ensureWritableSize(10);   //! Ԥ��10���ֽ�
 *  memset(b.writableBegin(), 0xcc, 10);    //! ����10���ֽ�ȫ��Ϊ0xcc
 *  b.hasWritten(10);           //! �����д��10���ֽ�
 *
 * \warnning    ���߳�ʹ�������ⲿ����
 */
class Buffer {
  public:
    static const size_t kInitialSize = 256;

    explicit Buffer(size_t reverse_size = kInitialSize);
    virtual ~Buffer();

    Buffer(const Buffer &other);
    Buffer(Buffer &&other);

    Buffer& operator = (const Buffer &other);
    Buffer& operator = (Buffer &&other);

    void swap(Buffer &other);
    void reset();

  public:
    /**
     * д�������
     */

    //! ��ȡ��д�ռ��С
    inline size_t writableSize() const { return buffer_size_ - write_index_; }

    //! ������ָ�������Ŀ�д�ռ�
    bool ensureWritableSize(size_t write_size);

    //! ��ȡд�׵�ַ
    inline uint8_t* writableBegin() const {
        return (buffer_ptr_ != nullptr) ? (buffer_ptr_ + write_index_) : nullptr;
    }

    //! �����д�����ݴ�С
    void hasWritten(size_t write_size);

    //! ��������׷��ָ�������ݿ飬����ʵ��д��Ĵ�С
    size_t append(const void *p_data, size_t data_size);

    /**
     * ���������
     */

    //! ��ȡ�ɶ������С
    inline size_t readableSize() const { return write_index_ - read_index_; }

    //! ��ȡ�ɶ����׵�ַ
    inline uint8_t* readableBegin() const {
        return (buffer_ptr_ != nullptr) ? (buffer_ptr_ + read_index_) : nullptr;
    }

    //! ����Ѷ����ݴ�С
    void hasRead(size_t read_size);

    //! ����Ѷ�ȡȫ������
    void hasReadAll();

    //! �ӻ�������ȡָ�������ݿ飬����ʵ�ʶ��������ݴ�С
    size_t fetch(void *p_buff, size_t buff_size);

    /**
     * ����
     */
    //! ���������������
    void shrink();

  protected:
    void cloneFrom(const Buffer &other);

  private:
    uint8_t *buffer_ptr_  = nullptr; //! ��������ַ
    size_t   buffer_size_ = 0;       //! ��������С

    size_t   read_index_  = 0;       //! ��λ��ƫ��
    size_t   write_index_ = 0;       //! дλ��ƫ��
};

}
}

#endif //TBOX_NETWORK_BUFFER_H_20171028
