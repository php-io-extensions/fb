<?php

declare(strict_types=1);

/*
 * Every stub declaration is what the loaded extension exposes: the stubs are
 * the source of truth, so a method or constant missing from the build fails here.
 */

function stubDeclarations(): array
{
    $declared = ['constants' => [], 'classes' => []];

    foreach (glob(__DIR__.'/../stubs/*.stub.php') as $stub) {
        $class = null;
        foreach (file($stub) as $line) {
            if (preg_match('/^(?:final\s+)?class\s+(\w+)/', $line, $m)) {
                $class = $m[1];
                $declared['classes'][$class] ??= [];
            } elseif (preg_match('/^const\s+(\w+)/', $line, $m)) {
                $declared['constants'][] = $m[1];
            } elseif ($class !== null && preg_match('/^\s+public\s+function\s+(\w+)/', $line, $m)) {
                $declared['classes'][$class][] = $m[1];
            }
        }
    }

    return $declared;
}

it('exposes every constant, class and method the stubs declare', function (): void {
    $declared = stubDeclarations();

    expect($declared['constants'])->toHaveCount(23)
        ->and(array_keys($declared['classes']))->toBe(['FbBuffer', 'FbFormat']);

    foreach ($declared['constants'] as $constant) {
        expect(defined($constant))->toBeTrue("{$constant} is missing");
    }
    foreach ($declared['classes'] as $class => $methods) {
        foreach ($methods as $method) {
            expect(method_exists($class, $method))->toBeTrue("{$class}::{$method}() is missing");
        }
    }
});

it('reports its version', function (): void {
    expect(phpversion('fb'))->toBe('0.10.0');
});

it('declares final classes that cannot be cloned or serialized', function (string $class): void {
    $object = $class === FbFormat::class ? new FbFormat(FB_LAYOUT_RGB888) : buffer(FB_LAYOUT_RGB888, 1, 1);

    expect((new ReflectionClass($class))->isFinal())->toBeTrue()
        ->and(fn () => clone $object)->toThrow(Error::class)
        ->and(fn () => serialize($object))->toThrow(Exception::class);
})->with([FbFormat::class, FbBuffer::class]);

it('refuses a second construction', function (): void {
    $format = new FbFormat(FB_LAYOUT_RGB888);
    $buffer = new FbBuffer($format, 2, 2);

    expect(fn () => $format->__construct(FB_LAYOUT_RGB565))->toThrow(Error::class, 'already constructed')
        ->and(fn () => $buffer->__construct($format, 4, 4))->toThrow(Error::class, 'already constructed')
        ->and($buffer->width())->toBe(2);
});
