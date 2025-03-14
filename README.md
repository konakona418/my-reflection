# my-reflection

This is a tiny C++ library which provides a simple reflection API.

This project is mainly made for me to learn template usage, and relies heavily on them,
which lacks stability and readability guarantees,
so I don't think it's a good idea to use it in production.

The development is still under progress, so the API might change in the future.
However, due to heavy schoolwork, I guess I will have limited time to work on this project. 😭

>
> I guess some might argue that this is not reflection.
> Well that depends on your definition of reflection.
> Whatever. 😋
> 

## Features
It provides several features:

* Access to class members and methods.
* Invoking functions.
* Type safety (I guess it does) guarantees.
* Limited support for inheritance.
* Limited support for templates.

All done during runtime, (partially) type-erased, and without any additional dependencies.

## Limitations
However, it does not provide the following features:

* Access to pure virtual methods (may lead to undefined behavior).
* Access to private or protected members & methods.
* Access to static members & methods.
* Type information access.

Also, it requires you to register all classes and fields you want to access manually,
which is somehow a bit inconvenient, but it's the only way to do it without any additional dependencies.

Maybe I will make another project to make it possible to automatically register classes and fields,
but I'm not sure.

## Usage
The library is header-only, so you just need to include the header file in your project.

Btw I made another helper library which can be used to write tests in such manner,

```c++
begin_test("my_test") {
    test(/* func...*/ );
    test(/* func...*/ );
} end_test()
```

You can use it if you like, for some simple tests.