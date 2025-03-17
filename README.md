# my-reflection

A lightweight C++ library providing simple reflection APIs.

This project was created primarily to practice template programming. Since it relies heavily on templates, it lacks stability and readability guarantees. As a result, I wouldn’t recommend using it in production.

Development is still ongoing, and the APIs may change in the future.

Due to a heavy academic workload, I might not have much time to work on this project. 😭

>
> Some might argue that this isn’t true reflection.
> Well, that depends on how you define reflection.
> Whatever. 😋
>

## Features

This library provides:

* Access to class members and methods
* Function invocation
* (Presumed) type safety guarantees
* Limited support for inheritance
* Limited support for templates
* Type information retrieval

All of this is done at runtime, with partial type erasure, and without any additional dependencies.

## Limitations

However, it does **not** support:

* Access to pure virtual methods (may lead to undefined behavior)
* Access to private or protected members and methods
* Access to static members and methods

Additionally, all classes and fields must be registered manually before use. While this is somewhat inconvenient, it is the only way to achieve reflection without introducing extra dependencies.

I may create another project to enable automatic registration of classes and fields, but I’m not sure yet.

## Usage

This is a header-only library, so you just need to include the header file in your project.

You can find some examples in the `examples` directory.

By the way, I also created a small helper library for writing tests in a structured way:

```c++
begin_test("my_test") {
    test(/* func... */);
    test(/* func... */);
} end_test()
```

Feel free to use it for simple testing if you find it helpful.

## License

This project is licensed under the GNU Affero General Public License v3.0.

See `LICENSE` for more details.
