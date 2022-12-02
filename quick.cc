

template <int i>
int func2() {
    static_assert(i < 2);

    return 0;
}

int func() {
    if constexpr (0 > 1)
        return 0;
    else
        return func2<2>();
}

int main() {
    return func();
}