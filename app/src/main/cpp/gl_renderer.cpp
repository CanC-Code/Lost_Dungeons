void GLRenderer::drawCompassHUD(float engineTime) {
    if (!uiShaderProgram) return;

    static std::vector<GLfloat> compVerts;
    static std::vector<GLuint> compIndices;
    static bool initCompassMesh = false;

    if (!initCompassMesh) {
        compVerts.insert(compVerts.end(), { 0.0f, 0.0f, 0.8f, 0.8f, 0.8f, 1.0f });

        const float PI = 3.14159265359f;
        int ringSegments = 36;
        float ringRadius = 1.0f;
        float ringInnerRadius = 0.85f;

        for (int i = 0; i < 16; ++i) {
            float angle = i * (PI * 2.0f / 16.0f) - (PI / 2.0f);
            float radius; float r, g, b;
            if (i == 0) { radius = 0.8f; r = 0.9f; g = 0.1f; b = 0.1f; }
            else if (i % 4 == 0) { radius = 0.8f; r = 0.8f; g = 0.8f; b = 0.8f; }
            else if (i % 2 == 0) { radius = 0.5f; r = 0.6f; g = 0.6f; b = 0.6f; }
            else { radius = 0.2f; r = 0.2f; g = 0.2f; b = 0.2f; }

            compVerts.insert(compVerts.end(), {
                cos(angle) * radius, sin(angle) * radius, r, g, b, 1.0f
            });
        }
        for (int i = 1; i <= 16; ++i) {
            int next = (i == 16) ? 1 : i + 1;
            compIndices.insert(compIndices.end(), { 0u, (GLuint)i, (GLuint)next });
        }

        int ringStartIdx = (int)(compVerts.size() / 6);
        for (int i = 0; i <= ringSegments; ++i) {
            float angle = i * (PI * 2.0f / ringSegments);
            compVerts.insert(compVerts.end(), {
                cos(angle) * ringInnerRadius, sin(angle) * ringInnerRadius, 0.6f, 0.5f, 0.1f, 1.0f
            });
            compVerts.insert(compVerts.end(), {
                cos(angle) * ringRadius, sin(angle) * ringRadius, 0.9f, 0.8f, 0.2f, 1.0f
            });
        }
        for (int i = 0; i < ringSegments; ++i) {
            GLuint inner1 = ringStartIdx + (i * 2);
            GLuint outer1 = inner1 + 1;
            GLuint inner2 = inner1 + 2;
            GLuint outer2 = inner1 + 3;
            compIndices.insert(compIndices.end(), {
                inner1, outer1, inner2, outer1, outer2, inner2
            });
        }
        initCompassMesh = true;
    }

    glDisable(GL_DEPTH_TEST);
    glUseProgram(uiShaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    glm::mat4 baseCompassModel = glm::translate(glm::mat4(1.0f),
        glm::vec3(COMPASS_CX, COMPASS_CY, 0.0f));

    float targetAngle = compassLockedToNorth ? 0.0f : glm::radians(yaw + 90.0f);
    float dt = engineTime - lastFrameTime;
    if (dt > 0.0f) {
        if (dt > 0.1f) dt = 0.1f;
        float diff = targetAngle - currentCompassAngle;
        while (diff > M_PI) diff -= 2.0f * M_PI;
        while (diff < -M_PI) diff += 2.0f * M_PI;
        float springTension = 30.0f, damping = 3.5f;
        compassVelocity += (diff * springTension - compassVelocity * damping) * dt;
        currentCompassAngle += compassVelocity * dt;
        lastFrameTime = engineTime;
    }

    float wobble = sin(engineTime * 35.0f) * 0.015f * std::min(std::abs(compassVelocity), 3.0f);
    float finalAngle = currentCompassAngle + wobble;

    // Draw needle and gold ring
    glm::mat4 needleModel = glm::rotate(baseCompassModel, finalAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    needleModel = glm::scale(needleModel, glm::vec3(60.0f, 60.0f, 1.0f));
    glm::mat4 mvp = projection * needleModel;
    glUniformMatrix4fv(uiMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), compVerts.data());
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), compVerts.data() + 2);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawElements(GL_TRIANGLES, (GLsizei)compIndices.size(), GL_UNSIGNED_INT, compIndices.data());

    // Recalculate timeOfDayAngle for the clock diamond
    float timeOfDayAngle = (engineTime / 3000.0f) * 3.14159265f * 2.0f;
    float sunX = cos(timeOfDayAngle) * 78.0f;
    float sunY = -sin(timeOfDayAngle) * 78.0f;
    glm::mat4 clockModel = glm::translate(baseCompassModel, glm::vec3(sunX, sunY, 0.0f));
    clockModel = glm::scale(clockModel, glm::vec3(12.0f, 12.0f, 1.0f));
    glm::mat4 clockMvp = projection * clockModel;
    glUniformMatrix4fv(uiMvpLoc, 1, GL_FALSE, glm::value_ptr(clockMvp));

    GLfloat clockVerts[] = {
        0.0f, -1.0f, 1.0f, 0.9f, 0.2f, 1.0f,
        -1.0f, 0.0f, 1.0f, 0.8f, 0.1f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.8f, 0.1f, 1.0f,
        0.0f, 1.0f, 1.0f, 0.9f, 0.2f, 1.0f
    };
    GLuint clockInds[] = { 0, 1, 2, 1, 3, 2 };

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), clockVerts);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), clockVerts + 2);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, clockInds);

    glEnable(GL_DEPTH_TEST);
}