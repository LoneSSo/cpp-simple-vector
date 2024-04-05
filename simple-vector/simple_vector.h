#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <utility>


#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj() = default;

    explicit ReserveProxyObj(size_t capacity_to_reserve)
        : value(capacity_to_reserve) {
    }

    size_t GetValue() const {
        return value;
    }
    
private:
    size_t value = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}; 

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) 
        : SimpleVector(size, Type{}) {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : storage_(size)
        , size_(size)
        , capacity_(size) {
        std::fill(storage_.Get(), &storage_[size], value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
        : storage_(init.size()) 
        , size_(init.size())
        , capacity_(init.size()) {
        std::copy(init.begin(), init.end(), storage_.Get());
    }

    // Создаёт вектор с заданной вместимостью
    SimpleVector(ReserveProxyObj capacity_to_reserve){
        Reserve(capacity_to_reserve.GetValue());
    }

    // Копирующий конструктор
    SimpleVector(const SimpleVector& other) 
        : storage_(other.size_)
        , size_(other.size_)
        , capacity_(other.size_) {
        std::copy(other.begin(), other.end(), storage_.Get());
    }

    // Перемещающий конструктор
    SimpleVector(SimpleVector&& other) 
        : storage_(std::move(other.storage_))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.size_, 0)) {
    }

    // Копирующее присваивание
    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs){
            SimpleVector<Type> tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    // Перемещающее присваивание
    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs){
            SimpleVector<Type> tmp(std::move(rhs));
            swap(tmp);
        }
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_){
            throw std::out_of_range("out_of_range");
        }
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_){
            throw std::out_of_range("out_of_range");
        }
        return storage_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер вектора. При увеличении размера заполняет «пустые» слоты дефолтными Type
    void Resize(size_t new_size) {
        if (new_size > capacity_){
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_;
            while (new_size > new_capacity){
                new_capacity *= 2;
            }
            ArrayPtr<Type> tmp(new_capacity);

            std::move(storage_.Get(), &storage_[size_], tmp.Get());
            FillWithDefault(&tmp[size_], &tmp[new_capacity]);
            
            storage_.swap(tmp);

            size_ = new_size;
            capacity_ = new_capacity;
            return;
        } 

        if(new_size > size_){
            FillWithDefault(&storage_[size_], &storage_[new_size]);
            size_ = new_size;
            return;
        }

        size_ = new_size;            
    }    

    // Задаёт вместимость вектора. Если заданная вместимость меньше текущей, то не делает ничего
    void Reserve(size_t new_capacity){
        if (new_capacity > capacity_){
            ArrayPtr<Type> tmp(new_capacity);
            std::move(begin(), end(), tmp.Get());
            storage_.swap(tmp);
            capacity_ = new_capacity;
        }
    }; 

    // Основан на механие Insert. Обеспечивает базовую гарантию безопасности.
    // Допустимо ли оставить эту механику, чтобы не дублировать (почти) код? 
    void PushBack(const Type& item) {
        Insert(end(), item);
        /*
        if(size_ == capacity_){
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);

            std::move(begin(), end(), tmp.Get());
            tmp[size_] = item;
            storage_.swap(tmp);
            ++size_;
            capacity_ = new_capacity;
            return;
        }

        ArrayPtr<Type> tmp(capacity_);
        std::move(begin(), end(), tmp.Get());
        tmp[size_] = item;
        storage_.swap(tmp);
        ++size_;
        */       
    }

    // Перемещающий PushBack
    void PushBack(Type&& item){
        Insert(end(), std::move(item));
        /*
        if(size_ == capacity_){
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);

            std::move(begin(), end(), tmp.Get());
            tmp[size_] = std::move(item);
            storage_.swap(tmp);
            ++size_;
            capacity_ = new_capacity;
            return;
        }

        ArrayPtr<Type> tmp(capacity_);
        std::move(begin(), end(), tmp.Get());
        tmp[size_] = std::move(item);
        storage_.swap(tmp);
        ++size_;
        */       
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
   Iterator Insert(ConstIterator pos, const Type& value) {
        
        size_t distance = static_cast<size_t>(std::distance(begin(), const_cast<Type*>(pos)));

        if(size_ == capacity_){
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);

            std::move(begin(), &storage_[distance], tmp.Get());
            std::exchange(tmp[distance], value);
            std::move(&storage_[distance], end(), &tmp[distance + 1]);

            storage_.swap(tmp);
            ++size_;
            capacity_ = new_capacity; 

            return &storage_[distance];    
        }

        std::move_backward(&storage_[distance], end(), end() + 1);
        std::exchange(storage_[distance], value);
        ++size_;

        return &storage_[distance];
    }

    // Перемещающий Insert
    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type&& value) {
        
        size_t distance = static_cast<size_t>(std::distance(begin(), const_cast<Type*>(pos)));

        if(size_ == capacity_){
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> tmp(new_capacity);

            std::move(begin(), &storage_[distance], tmp.Get());
            std::exchange(tmp[distance], std::move(value));
            std::move(&storage_[distance], end(), &tmp[distance + 1]);

            storage_.swap(tmp);
            ++size_;
            capacity_ = new_capacity; 

            return &storage_[distance];    
        }

        std::move_backward(&storage_[distance], end(), end() + 1);
        std::exchange(storage_[distance], std::move(value));
        ++size_;

        return &storage_[distance];
    }
    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        size_t distance = static_cast<size_t>(std::distance(begin(), const_cast<Type*>(pos)));

        std::move(&storage_[distance + 1], end(), &storage_[distance]);
        --size_;
        return &storage_[distance];       
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        storage_.swap(other.storage_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return &storage_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return cend();
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return &storage_[size_];
    }
private:
    // Вспомогательная функция. Заполняет диапазон дефолтными Type обходя noncopiable присваивание
    void FillWithDefault(Iterator first, Iterator last){
        for (auto it = first; it != last; it++){
            *it = std::move(Type{});
        }
    }

    ArrayPtr<Type> storage_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs <= lhs;
} 