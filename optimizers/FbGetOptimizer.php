<?php

namespace Zephir\Optimizers\FunctionCall;

use Zephir\Call;
use Zephir\CompilationContext;
use Zephir\CompiledExpression;
use Zephir\Exception\CompilerException;
use Zephir\Optimizers\OptimizerAbstract;

class FbGetOptimizer extends OptimizerAbstract
{
    public function optimize(array $expression, Call $call, CompilationContext $context)
    {
        if (!isset($expression['parameters']) || count($expression['parameters']) !== 3) {
            throw new CompilerException("'fb_get' requires exactly three parameters", $expression);
        }
        $resolvedParams = $call->getReadOnlyResolvedParams($expression['parameters'], $context, $expression);
        $context->headersManager->add('src/fb-api');

        return new CompiledExpression('long', 'fb_api_get(' . implode(', ', $resolvedParams) . ')', $expression);
    }
}
