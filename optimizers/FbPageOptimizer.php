<?php

namespace Zephir\Optimizers\FunctionCall;

use Zephir\Call;
use Zephir\CompilationContext;
use Zephir\CompiledExpression;
use Zephir\Exception\CompilerException;
use Zephir\Optimizers\OptimizerAbstract;

class FbPageOptimizer extends OptimizerAbstract
{
    public function optimize(array $expression, Call $call, CompilationContext $context)
    {
        if (!isset($expression['parameters']) || count($expression['parameters']) !== 1) {
            throw new CompilerException("'fb_page' requires exactly one parameter", $expression);
        }
        $resolvedParams = $call->getReadOnlyResolvedParams($expression['parameters'], $context, $expression);
        $context->headersManager->add('src/fb-api');

        return new CompiledExpression('long', 'fb_api_page(' . implode(', ', $resolvedParams) . ')', $expression);
    }
}
