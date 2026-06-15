/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef AKRINGBUFFER_H
#define AKRINGBUFFER_H

#include <algorithm>
#include <stdexcept>
#include <type_traits>

#include <QAtomicInt>
#include <QIODevice>
#include <QVector>

#include "akcommons.h"

// Lock-free single-producer / single-consumer ring buffer.
//
// Constraints:
//   - T must be trivially copyable and default constructible.
//   - Internal capacity is always rounded up to the next power of two.
//   - Only one thread may call write() at a time (producer).
//   - Only one thread may call read() at a time (consumer).
//   - Producer and consumer may run concurrently without a mutex.
//
// The class also implements QIODevice so it can interoperate with any Qt
// API that accepts a QIODevice (e.g. QAudioSink / QAudioSource).

template <typename T>
class AKCOMMONS_EXPORT AkRingBuffer: public QIODevice
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "AkRingBuffer requires a trivially copyable type");
    static_assert(std::is_default_constructible_v<T>,
                  "AkRingBuffer requires a default constructible type");

    public:
        // Construction / destruction

        inline explicit AkRingBuffer(int capacity=0, QObject *parent=nullptr):
            QIODevice(parent)
        {
            this->open(QIODevice::ReadWrite);

            if (capacity > 0)
                this->allocate(capacity);
        }

        inline AkRingBuffer(const AkRingBuffer &other):
            QIODevice()
        {
            this->open(QIODevice::ReadWrite);

            if (other.m_capacity > 0) {
                this->allocate(other.m_capacity);
                std::memcpy(this->m_data,
                            other.m_data,
                            size_t(other.m_capacity) * sizeof(T));
            }

            this->m_readPos.storeRelaxed(other.m_readPos.loadRelaxed());
            this->m_writePos.storeRelaxed(other.m_writePos.loadRelaxed());
        }

        inline AkRingBuffer(AkRingBuffer &&other) noexcept:
            QIODevice()
        {
            this->open(QIODevice::ReadWrite);
            this->m_data     = other.m_data;
            this->m_capacity = other.m_capacity;
            this->m_mask     = other.m_mask;
            this->m_readPos.storeRelaxed(other.m_readPos.loadRelaxed());
            this->m_writePos.storeRelaxed(other.m_writePos.loadRelaxed());

            other.m_data     = nullptr;
            other.m_capacity = 0;
            other.m_mask     = 0;
            other.m_readPos.storeRelaxed(0);
            other.m_writePos.storeRelaxed(0);
        }

        inline ~AkRingBuffer() override
        {
            delete[] this->m_data;
        }

        // Assignment

        inline AkRingBuffer &operator =(const AkRingBuffer &other)
        {
            if (this != &other) {
                AkRingBuffer tmp(other);
                this->swap(tmp);
            }

            return *this;
        }

        inline AkRingBuffer &operator =(AkRingBuffer &&other) noexcept
        {
            if (this != &other) {
                delete[] this->m_data;
                this->m_data     = other.m_data;
                this->m_capacity = other.m_capacity;
                this->m_mask     = other.m_mask;
                this->m_readPos.storeRelaxed(other.m_readPos.loadRelaxed());
                this->m_writePos.storeRelaxed(other.m_writePos.loadRelaxed());

                other.m_data     = nullptr;
                other.m_capacity = 0;
                other.m_mask     = 0;
                other.m_readPos.storeRelaxed(0);
                other.m_writePos.storeRelaxed(0);
            }

            return *this;
        }

        // Comparison

        inline bool operator ==(const AkRingBuffer &other) const
        {
            if (this == &other)
                return true;

            int avail = this->availableRead();

            if (avail != other.availableRead())
                return false;

            if (avail == 0)
                return true;

            QVector<T> a(avail), b(avail);
            const_cast<AkRingBuffer *>(this)->peek(a.data(), avail);
            const_cast<AkRingBuffer *>(&other)->peek(b.data(), avail);

            return std::equal(a.constBegin(), a.constEnd(), b.constBegin());
        }

        inline bool operator !=(const AkRingBuffer &other) const
        {
            return !(*this == other);
        }

        inline bool operator <(const AkRingBuffer &other) const
        {
            return this->availableRead() < other.availableRead();
        }

        inline bool operator <=(const AkRingBuffer &other) const
        {
            return this->availableRead() <= other.availableRead();
        }

        inline bool operator >(const AkRingBuffer &other) const
        {
            return this->availableRead() > other.availableRead();
        }

        inline bool operator >=(const AkRingBuffer &other) const
        {
            return this->availableRead() >= other.availableRead();
        }

        inline explicit operator bool() const
        {
            return !this->isEmpty();
        }

        // Concatenation / stream operators

        inline AkRingBuffer operator +(const AkRingBuffer &other) const
        {
            AkRingBuffer result(this->m_capacity + other.m_capacity);
            int aSize = this->availableRead();
            int bSize = other.availableRead();
            QVector<T> tmp(qMax(aSize, bSize));

            const_cast<AkRingBuffer *>(this)->peek(tmp.data(), aSize);
            result.write(tmp.constData(), aSize);

            const_cast<AkRingBuffer *>(&other)->peek(tmp.data(), bSize);
            result.write(tmp.constData(), bSize);

            return result;
        }

        inline AkRingBuffer &operator +=(const AkRingBuffer &other)
        {
            int sz = other.availableRead();
            QVector<T> tmp(sz);
            const_cast<AkRingBuffer *>(&other)->peek(tmp.data(), sz);
            this->write(tmp.constData(), sz);

            return *this;
        }

        inline AkRingBuffer &operator +=(const T &element)
        {
            this->write(element);

            return *this;
        }

        inline AkRingBuffer &operator <<(const T &element)
        {
            this->write(element);

            return *this;
        }

        inline AkRingBuffer &operator <<(const QVector<T> &elements)
        {
            this->write(elements.constData(), elements.size());

            return *this;
        }

        inline AkRingBuffer &operator >>(T &element)
        {
            element = this->read();

            return *this;
        }

        inline AkRingBuffer &operator >>(QVector<T> &elements)
        {
            int avail = this->availableRead();

            if (avail > 0) {
                int old = elements.size();
                elements.resize(old + avail);
                this->read(elements.data() + old, avail);
            }

            return *this;
        }

        // Indexed access (relative to current read position)

        inline T &operator [](int index)
        {
            if (index < 0 || index >= this->m_capacity)
                throw std::out_of_range("AkRingBuffer: index out of range");

            return this->m_data[(this->m_readPos.loadRelaxed() + index) & this->m_mask];
        }

        inline const T &operator [](int index) const
        {
            if (index < 0 || index >= this->m_capacity)
                throw std::out_of_range("AkRingBuffer: index out of range");

            return this->m_data[(this->m_readPos.loadRelaxed() + index) & this->m_mask];
        }

        inline T at(int index) const
        {
            if (index < 0 || index >= this->availableRead())
                throw std::out_of_range("AkRingBuffer: index out of range");

            return (*this)[index];
        }

        inline T value(int index, const T &defaultValue = T{}) const
        {
            if (index < 0 || index >= this->availableRead())
                return defaultValue;

            return this->m_data[(this->m_readPos.loadRelaxed() + index) & this->m_mask];
        }

        // Allocation

        // (Re)allocate the buffer to hold at least 'capacity' elements.
        // Rounds up to the next power of two. Discards any existing data.
        inline void allocate(int capacity)
        {
            int p = 1;

            while (p < capacity)
                p <<= 1;

            delete[] this->m_data;
            this->m_data     = new T[p]();
            this->m_capacity = p;
            this->m_mask     = p - 1;
            this->m_readPos.storeRelaxed(0);
            this->m_writePos.storeRelaxed(0);
        }

        // State queries

        inline int capacity()const
        {
            return this->m_capacity;

        }

        inline bool isEmpty() const
        {
            return this->availableRead() == 0;
        }

        inline bool isFull() const
        {
            return this->availableWrite() == 0;
        }

        inline int availableRead() const
        {
            return (this->m_writePos.loadAcquire()
                    - this->m_readPos.loadAcquire()) & this->m_mask;
        }

        inline int availableWrite() const
        {
            return this->m_capacity - 1 - this->availableRead();
        }

        // Write

        // Write up to nElements from src. Returns elements actually written.
        inline int write(const T *src, int nElements)
        {
            if (!src || nElements <= 0)
                return 0;

            int toWrite = qMin(nElements, this->availableWrite());
            int wp      = this->m_writePos.loadRelaxed() & this->m_mask;

            for (int i = 0; i < toWrite; ++i) {
                this->m_data[wp] = src[i];
                wp = (wp + 1) & this->m_mask;
            }

            this->m_writePos.fetchAndAddRelease(toWrite);

            return toWrite;
        }

        // Write a single element. Returns true on success.
        inline bool write(const T &element)
        {
            return this->write(&element, 1) == 1;
        }

        // Read

        // Read up to nElements into dst. Returns elements actually read.
        // Does NOT fill the remainder with a default value.
        inline int read(T *dst, int nElements)
        {
            if (!dst || nElements <= 0)
                return 0;

            int toRead = qMin(nElements, this->availableRead());
            int rp     = this->m_readPos.loadRelaxed() & this->m_mask;

            for (int i = 0; i < toRead; ++i) {
                dst[i] = this->m_data[rp];
                rp = (rp + 1) & this->m_mask;
            }

            this->m_readPos.fetchAndAddRelease(toRead);

            return toRead;
        }

        // Read exactly nElements into dst, filling any underrun with
        // defaultValue.
        inline int read(T *dst, int nElements, const T &defaultValue)
        {
            int got = this->read(dst, nElements);

            for (int i = got; i < nElements; ++i)
                dst[i] = defaultValue;

            return got;
        }

        // Read and return a single element (returns T{} if empty).
        inline T read()
        {
            T element{};
            this->read(&element, 1);

            return element;
        }

        // Peek at up to nElements without consuming them. Returns elements
        // peeked.
        inline int peek(T *dst, int nElements) const
        {
            if (!dst || nElements <= 0)
                return 0;

            int toPeek = qMin(nElements, this->availableRead());
            int rp     = this->m_readPos.loadAcquire() & this->m_mask;

            for (int i = 0; i < toPeek; ++i) {
                dst[i] = this->m_data[rp];
                rp = (rp + 1) & this->m_mask;
            }

            return toPeek;
        }

        // Convenience: peek at the next element to be read.
        inline T peek(const T &defaultValue = T{}) const
        {
            if (this->isEmpty())
                return defaultValue;

            return this->m_data[this->m_readPos.loadRelaxed() & this->m_mask];
        }

        inline T first(const T &defaultValue = T{}) const
        {
            return this->peek(defaultValue);
        }

        inline T last(const T &defaultValue = T{}) const
        {
            if (this->isEmpty())
                return defaultValue;

            return this->m_data[(this->m_writePos.loadRelaxed() - 1) & this->m_mask];
        }

        // -----------------------------------------------------------------------
        // Search
        // -----------------------------------------------------------------------

        inline bool contains(const T &element, int from = 0) const
        {
            return this->indexOf(element, from) >= 0;
        }

        inline int indexOf(const T &element, int from = 0) const
        {
            int avail = this->availableRead();

            if (from < 0) from = 0;

            for (int i = from; i < avail; ++i)
                if (this->value(i) == element)
                    return i;

            return -1;
        }

        inline int lastIndexOf(const T &element, int from = -1) const
        {
            int avail = this->availableRead();

            if (avail == 0)
                return -1;

            int start = (from < 0) ? avail - 1 : qMin(from, avail - 1);

            for (int i = start; i >= 0; --i)
                if (this->value(i) == element)
                    return i;

            return -1;
        }

        // Utility

        // Discard up to nElements. Returns elements actually discarded.
        inline int discard(int nElements)
        {
            int toDiscard = qMin(nElements, this->availableRead());
            this->m_readPos.fetchAndAddRelease(toDiscard);

            return toDiscard;
        }

        inline void clear()
        {
            this->m_readPos.storeRelaxed(0);
            this->m_writePos.storeRelaxed(0);
        }

        inline void reset(bool zeroMemory = false)
        {
            this->clear();

            if (zeroMemory && this->m_data && this->m_capacity > 0)
                std::fill_n(this->m_data, this->m_capacity, T{});
        }

        inline void swap(AkRingBuffer &other) noexcept
        {
            std::swap(this->m_data,     other.m_data);
            std::swap(this->m_capacity, other.m_capacity);
            std::swap(this->m_mask,     other.m_mask);

            int rp = this->m_readPos.loadRelaxed();
            this->m_readPos.storeRelaxed(other.m_readPos.loadRelaxed());
            other.m_readPos.storeRelaxed(rp);

            int wp = this->m_writePos.loadRelaxed();
            this->m_writePos.storeRelaxed(other.m_writePos.loadRelaxed());
            other.m_writePos.storeRelaxed(wp);
        }

    private:
        T         *m_data     {nullptr};
        int        m_capacity {0};
        int        m_mask     {0};
        QAtomicInt m_readPos  {0};
        QAtomicInt m_writePos {0};

    protected:
        // QIODevice interface

        qint64 readData(char *data, qint64 maxSize) override
        {
            int n = int(maxSize / qint64(sizeof(T)));

            if (n <= 0 || !data)
                return 0;

            return qint64(this->read(reinterpret_cast<T *>(data), n)) * qint64(sizeof(T));
        }

        qint64 writeData(const char *data, qint64 maxSize) override
        {
            int n = int(maxSize / qint64(sizeof(T)));

            if (n <= 0 || !data)
                return 0;

            return qint64(this->write(reinterpret_cast<const T *>(data), n)) * qint64(sizeof(T));
        }

        bool isSequential() const override
        {
            return true;
        }

        qint64 bytesAvailable() const override
        {
            return qint64(this->availableRead()) * qint64(sizeof(T))
                   + QIODevice::bytesAvailable();
        }

        qint64 bytesToWrite() const override
        {
            return qint64(this->availableWrite()) * qint64(sizeof(T));
        }
};

#endif // AKRINGBUFFER_H
